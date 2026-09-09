// SecondScreen NVIDIA NVENC Hardware Video Encoder
// Dynamic loader for nvEncodeAPI64.dll with ultra-low-latency CBR zero-copy pipeline
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#include "HardwareEncoder.h"
#include "../vendor/nvidia/nvEncodeAPI.h"
#include <unordered_map>

namespace SecondScreen {

    class NVENCEncoder : public IHardwareEncoder {
    public:
        NVENCEncoder();
        virtual ~NVENCEncoder();

        static bool IsSupported(ID3D11Device* pDevice);

        virtual bool Initialize(ID3D11Device* pDevice, const EncoderConfig& config) override;
        virtual bool Reconfigure(const EncoderConfig& config) override;
        virtual bool EncodeTexture(ID3D11Texture2D* pTexture, uint64_t frameNumber, uint64_t captureTimestampUs, bool forceKeyFrame, EncodedVideoPacket* pOutPacket) override;
        virtual void Flush(std::vector<EncodedVideoPacket>& outPackets) override;
        virtual void Shutdown() override;

        virtual EncoderBackend GetBackendType() const override { return EncoderBackend::NVENC; }
        virtual const char* GetBackendName() const override { return "NVIDIA NVENC (Hardware Accelerated)"; }
        virtual bool IsHardwareAccelerated() const override { return true; }

    private:
        bool LoadNvencLibrary();
        void UnloadNvencLibrary();
        bool CreateEncoderSession();
        bool AllocateBitstreamBuffers();
        void ReleaseBitstreamBuffers();
        void ReleaseRegisteredResources();

        HMODULE m_hNvencDll;
        NV_ENCODE_API_FUNCTION_LIST m_nvEnc;
        void* m_hEncoder;
        ID3D11Device* m_pDirect3DDevice;
        EncoderConfig m_Config;
        bool m_Initialized;

        static constexpr size_t NUM_BITSTREAM_BUFFERS = 3;
        NV_ENC_OUTPUT_PTR m_BitstreamBuffers[NUM_BITSTREAM_BUFFERS];
        size_t m_CurrentBitstreamBufferIdx;

        // Texture registration cache: texture pointer -> registered resource handle
        std::unordered_map<ID3D11Texture2D*, NV_ENC_REGISTERED_PTR> m_RegisteredResources;
    };

}

