// SecondScreen NVIDIA NVENC Hardware Video Encoder Implementation
// Genuine NVIDIA Video Codec SDK Pipeline (Zero-Copy D3D11 -> NVENC -> H.264 Annex-B)
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "NVENCEncoder.h"
#include <iostream>
#include <chrono>

namespace SecondScreen {

    NVENCEncoder::NVENCEncoder()
        : m_hNvencDll(nullptr),
          m_hEncoder(nullptr),
          m_pDirect3DDevice(nullptr),
          m_Initialized(false),
          m_CurrentBitstreamBufferIdx(0) {
        ZeroMemory(&m_nvEnc, sizeof(m_nvEnc));
        for (size_t i = 0; i < NUM_BITSTREAM_BUFFERS; ++i) {
            m_BitstreamBuffers[i] = nullptr;
        }
    }

    NVENCEncoder::~NVENCEncoder() {
        Shutdown();
    }

    bool NVENCEncoder::IsSupported(ID3D11Device* pDevice) {
        if (!pDevice) return false;

        HMODULE hModule = LoadLibraryW(L"nvEncodeAPI64.dll");
        if (!hModule) {
            return false;
        }

        typedef NVENCSTATUS(NVENCAPI* PNVENCODEAPICREATEINSTANCE)(NV_ENCODE_API_FUNCTION_LIST*);
        auto pfnCreateInstance = (PNVENCODEAPICREATEINSTANCE)GetProcAddress(hModule, "NvEncodeAPICreateInstance");
        if (!pfnCreateInstance) {
            FreeLibrary(hModule);
            return false;
        }

        NV_ENCODE_API_FUNCTION_LIST nvEncList = {};
        nvEncList.version = NV_ENCODE_API_FUNCTION_LIST_VER;
        NVENCSTATUS status = pfnCreateInstance(&nvEncList);
        if (status != NV_ENC_SUCCESS) {
            FreeLibrary(hModule);
            return false;
        }

        // Probe hardware encode session on the target D3D11 device
        NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS openParams = {};
        openParams.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
        openParams.device = pDevice;
        openParams.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
        openParams.apiVersion = NVENCAPI_VERSION;

        void* hTestEncoder = nullptr;
        status = nvEncList.nvEncOpenEncodeSessionEx(&openParams, &hTestEncoder);
        if (status == NV_ENC_SUCCESS && hTestEncoder) {
            nvEncList.nvEncDestroyEncoder(hTestEncoder);
            FreeLibrary(hModule);
            return true;
        }

        FreeLibrary(hModule);
        return false;
    }

    bool NVENCEncoder::LoadNvencLibrary() {
        if (m_hNvencDll) return true;

        m_hNvencDll = LoadLibraryW(L"nvEncodeAPI64.dll");
        if (!m_hNvencDll) {
            return false;
        }

        typedef NVENCSTATUS(NVENCAPI* PNVENCODEAPICREATEINSTANCE)(NV_ENCODE_API_FUNCTION_LIST*);
        auto pfnCreateInstance = (PNVENCODEAPICREATEINSTANCE)GetProcAddress(m_hNvencDll, "NvEncodeAPICreateInstance");
        if (!pfnCreateInstance) {
            UnloadNvencLibrary();
            return false;
        }

        m_nvEnc.version = NV_ENCODE_API_FUNCTION_LIST_VER;
        NVENCSTATUS status = pfnCreateInstance(&m_nvEnc);
        if (status != NV_ENC_SUCCESS) {
            std::wcerr << L"[NVENC] NvEncodeAPICreateInstance failed, status=" << status << std::endl;
            UnloadNvencLibrary();
            return false;
        }

        return true;
    }

    void NVENCEncoder::UnloadNvencLibrary() {
        if (m_hNvencDll) {
            FreeLibrary(m_hNvencDll);
            m_hNvencDll = nullptr;
        }
        ZeroMemory(&m_nvEnc, sizeof(m_nvEnc));
    }

    bool NVENCEncoder::CreateEncoderSession() {
        if (!m_pDirect3DDevice || !m_nvEnc.nvEncOpenEncodeSessionEx) return false;

        // 1. Open Encode Session
        NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS openParams = {};
        openParams.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
        openParams.device = m_pDirect3DDevice;
        openParams.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
        openParams.apiVersion = NVENCAPI_VERSION;

        NVENCSTATUS status = m_nvEnc.nvEncOpenEncodeSessionEx(&openParams, &m_hEncoder);
        if (status != NV_ENC_SUCCESS || !m_hEncoder) {
            std::wcerr << L"[NVENC] nvEncOpenEncodeSessionEx failed, status=" << status << std::endl;
            return false;
        }

        // 2. Query low-latency preset configuration
        GUID codecGuid = NV_ENC_CODEC_H264_GUID;
        GUID presetGuid = NV_ENC_PRESET_P1_GUID; // Ultra-fast low latency preset
        NV_ENC_TUNING_INFO tuningInfo = NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;

        NV_ENC_PRESET_CONFIG presetConfig = {};
        presetConfig.version = NV_ENC_PRESET_CONFIG_VER;
        presetConfig.presetCfg.version = NV_ENC_CONFIG_VER;

        status = m_nvEnc.nvEncGetEncodePresetConfigEx(m_hEncoder, codecGuid, presetGuid, tuningInfo, &presetConfig);
        if (status != NV_ENC_SUCCESS) {
            // Fallback to basic preset config if Ex is unsupported
            status = m_nvEnc.nvEncGetEncodePresetConfig(m_hEncoder, codecGuid, presetGuid, &presetConfig);
        }

        // 3. Configure Ultra-Low-Latency CBR Bitstream Parameters
        NV_ENC_CONFIG& encConfig = presetConfig.presetCfg;
        encConfig.gopLength = m_Config.gopSize;
        encConfig.frameIntervalP = 1; // 1 = IPPP... (Zero B-frames for minimal latency)

        // Rate control: Constant Bitrate (CBR)
        encConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
        encConfig.rcParams.averageBitRate = m_Config.bitrateKbps * 1000;
        encConfig.rcParams.maxBitRate = m_Config.maxBitrateKbps * 1000;
        encConfig.rcParams.vbvBufferSize = (m_Config.bitrateKbps * 1000) / m_Config.fps; // Single-frame VBV buffer
        encConfig.rcParams.vbvInitialDelay = encConfig.rcParams.vbvBufferSize;
        encConfig.rcParams.zeroReorderDelay = 1; // Immediate display order output

        // H.264 Specifics: Repeat SPS/PPS on IDR frames for instantaneous iPad receiver synchronization
        encConfig.encodeCodecConfig.h264Config.repeatSPSPPS = 1;
        encConfig.encodeCodecConfig.h264Config.idrPeriod = m_Config.gopSize;
        encConfig.encodeCodecConfig.h264Config.outputAnnexB = 1; // Standard 0x00000001 NALU stream
        encConfig.encodeCodecConfig.h264Config.level = NV_ENC_LEVEL_H264_51;
        encConfig.encodeCodecConfig.h264Config.h264VUIParameters.videoFullRangeFlag = 1;
        encConfig.encodeCodecConfig.h264Config.h264VUIParameters.colourPrimaries = 1; // BT.709
        encConfig.encodeCodecConfig.h264Config.h264VUIParameters.transferCharacteristics = 1; // BT.709
        encConfig.encodeCodecConfig.h264Config.h264VUIParameters.colourMatrix = 1; // BT.709

        // 4. Initialize Encoder Session
        NV_ENC_INITIALIZE_PARAMS initParams = {};
        initParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
        initParams.encodeGUID = codecGuid;
        initParams.presetGUID = presetGuid;
        initParams.encodeWidth = m_Config.width;
        initParams.encodeHeight = m_Config.height;
        initParams.darWidth = m_Config.width;
        initParams.darHeight = m_Config.height;
        initParams.frameRateNum = m_Config.fps;
        initParams.frameRateDen = 1;
        initParams.enablePTD = 1; // Driver picture-type decision
        initParams.encodeConfig = &encConfig;
        initParams.tuningInfo = tuningInfo;

        status = m_nvEnc.nvEncInitializeEncoder(m_hEncoder, &initParams);
        if (status != NV_ENC_SUCCESS) {
            std::wcerr << L"[NVENC] nvEncInitializeEncoder failed, status=" << status << std::endl;
            m_nvEnc.nvEncDestroyEncoder(m_hEncoder);
            m_hEncoder = nullptr;
            return false;
        }

        return true;
    }

    bool NVENCEncoder::AllocateBitstreamBuffers() {
        for (size_t i = 0; i < NUM_BITSTREAM_BUFFERS; ++i) {
            NV_ENC_CREATE_BITSTREAM_BUFFER createBuf = {};
            createBuf.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;

            NVENCSTATUS status = m_nvEnc.nvEncCreateBitstreamBuffer(m_hEncoder, &createBuf);
            if (status != NV_ENC_SUCCESS || !createBuf.bitstreamBuffer) {
                std::wcerr << L"[NVENC] nvEncCreateBitstreamBuffer failed on slot " << i << std::endl;
                return false;
            }
            m_BitstreamBuffers[i] = createBuf.bitstreamBuffer;
        }
        m_CurrentBitstreamBufferIdx = 0;
        return true;
    }

    void NVENCEncoder::ReleaseBitstreamBuffers() {
        if (!m_hEncoder || !m_nvEnc.nvEncDestroyBitstreamBuffer) return;

        for (size_t i = 0; i < NUM_BITSTREAM_BUFFERS; ++i) {
            if (m_BitstreamBuffers[i]) {
                m_nvEnc.nvEncDestroyBitstreamBuffer(m_hEncoder, m_BitstreamBuffers[i]);
                m_BitstreamBuffers[i] = nullptr;
            }
        }
    }

    void NVENCEncoder::ReleaseRegisteredResources() {
        if (!m_hEncoder || !m_nvEnc.nvEncUnregisterResource) return;

        for (auto& pair : m_RegisteredResources) {
            if (pair.second) {
                m_nvEnc.nvEncUnregisterResource(m_hEncoder, pair.second);
            }
        }
        m_RegisteredResources.clear();
    }

    bool NVENCEncoder::Initialize(ID3D11Device* pDevice, const EncoderConfig& config) {
        if (!pDevice) return false;
        Shutdown();

        m_pDirect3DDevice = pDevice;
        m_Config = config;

        if (!LoadNvencLibrary()) {
            std::wcerr << L"[NVENC] nvEncodeAPI64.dll failed to load." << std::endl;
            return false;
        }

        if (!CreateEncoderSession()) {
            UnloadNvencLibrary();
            return false;
        }

        if (!AllocateBitstreamBuffers()) {
            Shutdown();
            return false;
        }

        std::wcout << L"[NVENC] Hardware encoder initialized: " << m_Config.width << L"x" << m_Config.height 
                   << L" @ " << m_Config.fps << L"fps, " << (m_Config.bitrateKbps / 1000) 
                   << L" Mbps CBR (Ultra-Low-Latency H.264 Annex-B)" << std::endl;

        m_Initialized = true;
        return true;
    }

    bool NVENCEncoder::Reconfigure(const EncoderConfig& config) {
        if (!m_Initialized || !m_hEncoder || !m_nvEnc.nvEncReconfigureEncoder) return false;

        NV_ENC_RECONFIGURE_PARAMS reconfigParams = {};
        reconfigParams.version = NV_ENC_RECONFIGURE_PARAMS_VER;
        reconfigParams.reInitEncodeParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
        reconfigParams.reInitEncodeParams.encodeWidth = config.width;
        reconfigParams.reInitEncodeParams.encodeHeight = config.height;
        reconfigParams.reInitEncodeParams.darWidth = config.width;
        reconfigParams.reInitEncodeParams.darHeight = config.height;
        reconfigParams.reInitEncodeParams.frameRateNum = config.fps;
        reconfigParams.reInitEncodeParams.frameRateDen = 1;

        NV_ENC_CONFIG encConfig = {};
        encConfig.version = NV_ENC_CONFIG_VER;
        encConfig.gopLength = config.gopSize;
        encConfig.frameIntervalP = 1;
        encConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
        encConfig.rcParams.averageBitRate = config.bitrateKbps * 1000;
        encConfig.rcParams.maxBitRate = config.maxBitrateKbps * 1000;
        encConfig.encodeCodecConfig.h264Config.repeatSPSPPS = 1;
        encConfig.encodeCodecConfig.h264Config.idrPeriod = config.gopSize;
        encConfig.encodeCodecConfig.h264Config.outputAnnexB = 1;

        reconfigParams.reInitEncodeParams.encodeConfig = &encConfig;

        NVENCSTATUS status = m_nvEnc.nvEncReconfigureEncoder(m_hEncoder, &reconfigParams);
        if (status == NV_ENC_SUCCESS) {
            m_Config = config;
            return true;
        }

        // If in-place reconfigure is unsupported, full re-initialize
        return Initialize(m_pDirect3DDevice, config);
    }

    bool NVENCEncoder::EncodeTexture(ID3D11Texture2D* pTexture, uint64_t frameNumber, uint64_t captureTimestampUs, bool forceKeyFrame, EncodedVideoPacket* pOutPacket) {
        if (!m_Initialized || !m_hEncoder || !pTexture || !pOutPacket) return false;

        D3D11_TEXTURE2D_DESC desc;
        pTexture->GetDesc(&desc);

        NV_ENC_BUFFER_FORMAT encBufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
        if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM) {
            encBufferFmt = NV_ENC_BUFFER_FORMAT_ABGR;
        } else if (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM) {
            encBufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
        } else if (desc.Format == DXGI_FORMAT_NV12) {
            encBufferFmt = NV_ENC_BUFFER_FORMAT_NV12;
        }

        // 1. Register or retrieve cached direct GPU texture resource
        NV_ENC_REGISTERED_PTR registeredHandle = nullptr;
        auto it = m_RegisteredResources.find(pTexture);
        if (it != m_RegisteredResources.end()) {
            registeredHandle = it->second;
        } else {
            NV_ENC_REGISTER_RESOURCE regRes = {};
            regRes.version = NV_ENC_REGISTER_RESOURCE_VER;
            regRes.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
            regRes.resourceToRegister = pTexture;
            regRes.width = desc.Width;
            regRes.height = desc.Height;
            regRes.pitch = 0;
            regRes.bufferFormat = encBufferFmt;
            regRes.bufferUsage = NV_ENC_INPUT_IMAGE;

            NVENCSTATUS status = m_nvEnc.nvEncRegisterResource(m_hEncoder, &regRes);
            if (status != NV_ENC_SUCCESS) {
                std::wcerr << L"[NVENC] nvEncRegisterResource failed, status=" << status << std::endl;
                return false;
            }
            registeredHandle = regRes.registeredResource;
            m_RegisteredResources[pTexture] = registeredHandle;
        }

        // 2. Map input texture surface into NVENC hardware address space
        NV_ENC_MAP_INPUT_RESOURCE mapRes = {};
        mapRes.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
        mapRes.registeredResource = registeredHandle;

        NVENCSTATUS status = m_nvEnc.nvEncMapInputResource(m_hEncoder, &mapRes);
        if (status != NV_ENC_SUCCESS) {
            std::wcerr << L"[NVENC] nvEncMapInputResource failed, status=" << status << std::endl;
            return false;
        }

        // 3. Acquire next output bitstream buffer
        NV_ENC_OUTPUT_PTR outBitstream = m_BitstreamBuffers[m_CurrentBitstreamBufferIdx];
        m_CurrentBitstreamBufferIdx = (m_CurrentBitstreamBufferIdx + 1) % NUM_BITSTREAM_BUFFERS;

        // 4. Submit picture to hardware encoder pipeline
        NV_ENC_PIC_PARAMS picParams = {};
        picParams.version = NV_ENC_PIC_PARAMS_VER;
        picParams.inputBuffer = mapRes.mappedResource;
        picParams.bufferFmt = mapRes.mappedBufferFmt;
        picParams.inputWidth = desc.Width;
        picParams.inputHeight = desc.Height;
        picParams.outputBitstream = outBitstream;
        picParams.inputTimeStamp = captureTimestampUs;
        picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;

        bool isIdr = forceKeyFrame || (frameNumber % m_Config.gopSize == 0);
        if (isIdr) {
            picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR | NV_ENC_PIC_FLAG_OUTPUT_SPSPPS;
        }

        status = m_nvEnc.nvEncEncodePicture(m_hEncoder, &picParams);
        if (status != NV_ENC_SUCCESS) {
            std::wcerr << L"[NVENC] nvEncEncodePicture failed, status=" << status << std::endl;
            m_nvEnc.nvEncUnmapInputResource(m_hEncoder, mapRes.mappedResource);
            return false;
        }

        // 5. Lock encoded bitstream buffer and extract real H.264 Annex-B NAL units
        NV_ENC_LOCK_BITSTREAM lockParams = {};
        lockParams.version = NV_ENC_LOCK_BITSTREAM_VER;
        lockParams.outputBitstream = outBitstream;
        lockParams.doNotWait = 0; // Wait for hardware encode completion

        status = m_nvEnc.nvEncLockBitstream(m_hEncoder, &lockParams);
        if (status != NV_ENC_SUCCESS) {
            std::wcerr << L"[NVENC] nvEncLockBitstream failed, status=" << status << std::endl;
            m_nvEnc.nvEncUnmapInputResource(m_hEncoder, mapRes.mappedResource);
            return false;
        }

        // 6. Copy genuine H.264 bitstream data into output packet
        auto now = std::chrono::high_resolution_clock::now();
        uint64_t encodeTimestampUs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

        const uint8_t* pBitstreamData = static_cast<const uint8_t*>(lockParams.bitstreamBufferPtr);
        pOutPacket->data.assign(pBitstreamData, pBitstreamData + lockParams.bitstreamSizeInBytes);
        pOutPacket->isKeyFrame = (lockParams.pictureType == NV_ENC_PIC_TYPE_IDR || lockParams.pictureType == NV_ENC_PIC_TYPE_I || isIdr);
        pOutPacket->frameNumber = frameNumber;
        pOutPacket->captureTimestampUs = captureTimestampUs;
        pOutPacket->encodeTimestampUs = encodeTimestampUs;
        pOutPacket->width = m_Config.width;
        pOutPacket->height = m_Config.height;
        pOutPacket->codec = m_Config.codec;

        // 7. Release locks and unmap
        m_nvEnc.nvEncUnlockBitstream(m_hEncoder, outBitstream);
        m_nvEnc.nvEncUnmapInputResource(m_hEncoder, mapRes.mappedResource);

        return true;
    }

    void NVENCEncoder::Flush(std::vector<EncodedVideoPacket>& outPackets) {
        if (!m_Initialized || !m_hEncoder || !m_nvEnc.nvEncEncodePicture) return;

        NV_ENC_PIC_PARAMS picParams = {};
        picParams.version = NV_ENC_PIC_PARAMS_VER;
        picParams.encodePicFlags = NV_ENC_PIC_FLAG_EOS;

        m_nvEnc.nvEncEncodePicture(m_hEncoder, &picParams);
    }

    void NVENCEncoder::Shutdown() {
        if (m_Initialized) {
            ReleaseRegisteredResources();
            ReleaseBitstreamBuffers();

            if (m_hEncoder && m_nvEnc.nvEncDestroyEncoder) {
                m_nvEnc.nvEncDestroyEncoder(m_hEncoder);
                m_hEncoder = nullptr;
            }

            m_Initialized = false;
        }

        UnloadNvencLibrary();
    }

}

