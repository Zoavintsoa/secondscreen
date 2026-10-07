#include "H264Encoder.h"

#include <algorithm>
#include <cstring>
#include <mfapi.h>
#include <mferror.h>
#include <codecapi.h>
#include <wmcodecdsp.h>
#include <mfobjects.h>
#include <mftransform.h>
#include <combaseapi.h>

namespace second_screen {

namespace {
bool IsStartCode(const uint8_t* p, size_t remaining, size_t& length) {
    if (remaining >= 4 && p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1) {
        length = 4; return true;
    }
    if (remaining >= 3 && p[0] == 0 && p[1] == 0 && p[2] == 1) {
        length = 3; return true;
    }
    return false;
}
}

H264Encoder::~H264Encoder() {
    shutdown();
}

bool H264Encoder::initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrateKbps) {
    shutdown();
    if (!width || !height || !fps || !bitrateKbps) return false;

    HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) return false;

    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrateKbps_ = bitrateKbps;

    if (!configureD3DManager() || !createEncoder() || !configureEncoder()) {
        shutdown();
        return false;
    }

    started_ = true;
    return true;
}

bool H264Encoder::configureD3DManager() {
    HRESULT hr = MFCreateDXGIDeviceManager(&dxgiManagerResetToken_, &dxgiManager_);
    return SUCCEEDED(hr);
}

bool H264Encoder::createEncoder() {
    MFT_REGISTER_TYPE_INFO inputInfo{MFMediaType_Video, MFVideoFormat_NV12};
    MFT_REGISTER_TYPE_INFO outputInfo{MFMediaType_Video, MFVideoFormat_H264};

    IMFActivate** activates = nullptr;
    UINT32 count = 0;

    HRESULT hr = MFTEnumEx(
        MFT_CATEGORY_VIDEO_ENCODER,
        MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_SORTANDFILTER,
        &outputInfo,
        &inputInfo,
        &activates,
        &count);

    if (SUCCEEDED(hr) && count) {
        hr = activates[0]->ActivateObject(IID_PPV_ARGS(&transform_));
    }

    for (UINT32 i = 0; i < count; ++i) activates[i]->Release();
    CoTaskMemFree(activates);

    if (FAILED(hr) || !transform_) {
        // The inbox software encoder remains a valid fallback.
        hr = CoCreateInstance(
            CLSID_CMSH264EncoderMFT, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&transform_));
    }
    if (FAILED(hr) || !transform_) return false;

    transform_->QueryInterface(IID_PPV_ARGS(&codecApi_));
    return true;
}

bool H264Encoder::setCodecProperty(const GUID& property, VARIANT_BOOL value) {
    if (!codecApi_) return false;
    VARIANT v{};
    VariantInit(&v);
    v.vt = VT_BOOL;
    v.boolVal = value;
    HRESULT hr = codecApi_->SetValue(&property, &v);
    VariantClear(&v);
    return SUCCEEDED(hr);
}

bool H264Encoder::setCodecProperty(const GUID& property, uint32_t value) {
    if (!codecApi_) return false;
    VARIANT v{};
    VariantInit(&v);
    v.vt = VT_UI4;
    v.ulVal = value;
    HRESULT hr = codecApi_->SetValue(&property, &v);
    VariantClear(&v);
    return SUCCEEDED(hr);
}

bool H264Encoder::configureEncoder() {
    if (!transform_) return false;

    if (codecApi_) {
        setCodecProperty(CODECAPI_AVLowLatencyMode, VARIANT_TRUE);
        setCodecProperty(CODECAPI_AVEncCommonMeanBitRate, bitrateKbps_ * 1000u);
        setCodecProperty(CODECAPI_AVEncMPVGOPSize, fps_);
        setCodecProperty(CODECAPI_AVEncVideoForceKeyFrame, VARIANT_TRUE);
    }

    HRESULT hr = MFCreateMediaType(&outputType_);
    if (FAILED(hr)) return false;

    outputType_->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outputType_->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    outputType_->SetUINT32(MF_MT_AVG_BITRATE, bitrateKbps_ * 1000u);
    outputType_->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(outputType_.Get(), MF_MT_FRAME_SIZE, width_, height_);
    MFSetAttributeRatio(outputType_.Get(), MF_MT_FRAME_RATE, fps_, 1);
    MFSetAttributeRatio(outputType_.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = transform_->SetOutputType(0, outputType_.Get(), 0);
    if (FAILED(hr)) return false;

    UINT32 sequenceSize = 0;
    BYTE* sequenceData = nullptr;
    if (SUCCEEDED(outputType_->GetAllocatedBlob(
            MF_MT_MPEG_SEQUENCE_HEADER, &sequenceData, &sequenceSize))) {
        sequenceHeader_.assign(sequenceData, sequenceData + sequenceSize);
        CoTaskMemFree(sequenceData);
    }

    hr = MFCreateMediaType(&inputType_);
    if (FAILED(hr)) return false;

    inputType_->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inputType_->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    inputType_->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(inputType_.Get(), MF_MT_FRAME_SIZE, width_, height_);
    MFSetAttributeRatio(inputType_.Get(), MF_MT_FRAME_RATE, fps_, 1);
    MFSetAttributeRatio(inputType_.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = transform_->SetInputType(0, inputType_.Get(), 0);
    if (FAILED(hr)) return false;

    transform_->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(dxgiManager_.Get()));
    transform_->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    transform_->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

    return true;
}

bool H264Encoder::encode(ID3D11Texture2D* d3dTexture, const FrameInfo& info, EncodedAccessUnit& output) {
    if (!started_ || !d3dTexture) return false;

    if (!d3dConfigured_) {
        if (FAILED(d3dTexture->GetDevice(&d3dDevice_)) || !d3dDevice_) return false;
        if (!converter_.initialize(d3dDevice_.Get(), width_, height_)) return false;
        if (FAILED(dxgiManager_->ResetDevice(d3dDevice_.Get(), dxgiManagerResetToken_))) return false;
        transform_->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(dxgiManager_.Get()));
        d3dConfigured_ = true;
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> nv12Texture;
    if (!converter_.convert(d3dTexture, &nv12Texture)) return false;

    if (forceKeyFrame_) {
        if (codecApi_) setCodecProperty(CODECAPI_AVEncVideoForceKeyFrame, VARIANT_TRUE);
        forceKeyFrame_ = false;
    }

    Microsoft::WRL::ComPtr<IMFMediaBuffer> mediaBuffer;
    HRESULT hr = MFCreateDXGISurfaceBuffer(
        __uuidof(ID3D11Texture2D), nv12Texture.Get(), 0, FALSE, &mediaBuffer);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IMFSample> sample;
    hr = MFCreateSample(&sample);
    if (FAILED(hr)) return false;

    sample->AddBuffer(mediaBuffer.Get());
    sample->SetSampleTime(static_cast<LONGLONG>(info.timestampUs * 10));
    sample->SetSampleDuration(static_cast<LONGLONG>(10000000ull / fps_));

    hr = transform_->ProcessInput(0, sample.Get(), 0);
    if (FAILED(hr) && hr != MF_E_NOTACCEPTING) return false;

    bool produced = false;
    return drainOutput(output, produced) && produced;
}

bool H264Encoder::drainOutput(EncodedAccessUnit& output, bool& produced) {
    produced = false;
    MFT_OUTPUT_STREAM_INFO streamInfo{};
    if (FAILED(transform_->GetOutputStreamInfo(0, &streamInfo))) return false;

    std::vector<uint8_t> buffer(std::max<DWORD>(streamInfo.cbSize, 1024u));
    Microsoft::WRL::ComPtr<IMFMediaBuffer> outBuffer;
    HRESULT hr = MFCreateMemoryBuffer(static_cast<DWORD>(buffer.size()), &outBuffer);
    if (FAILED(hr)) return false);

    Microsoft::WRL::ComPtr<IMFSample> outSample;
    hr = MFCreateSample(&outSample);
    if (FAILED(hr)) return false;
    outSample->AddBuffer(outBuffer.Get());

    MFT_OUTPUT_DATA_BUFFER data{};
    data.dwStreamID = 0;
    data.pSample = outSample.Get();

    DWORD status = 0;
    hr = transform_->ProcessOutput(0, 1, &data, &status);
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) return true;
    if (FAILED(hr)) return false;

    BYTE* ptr = nullptr;
    DWORD maxLen = 0, currentLen = 0;
    hr = outBuffer->Lock(&ptr, &maxLen, &currentLen);
    if (FAILED(hr)) return false;

    output.annexB.clear();
    LONGLONG sampleTime = 0;
    output.timestampUs = SUCCEEDED(outSample->GetSampleTime(&sampleTime))
        ? static_cast<uint64_t>(sampleTime / 10) : 0;

    UINT32 clean = 0;
    output.keyFrame = SUCCEEDED(outSample->GetUINT32(MFSampleExtension_CleanPoint, &clean)) && clean != 0;
    if (!output.keyFrame && forceKeyFrame_) output.keyFrame = true;

    const bool ok = normalizeAnnexB(ptr, currentLen, output.annexB, output.keyFrame);
    outBuffer->Unlock();
    if (!ok) return false;

    produced = !output.annexB.empty();
    return produced;
}

bool H264Encoder::prependSequenceHeader(std::vector<uint8_t>& annexB) {
    if (sequenceHeader_.empty() || annexB.empty()) return true;
    std::vector<uint8_t> combined;
    combined.reserve(sequenceHeader_.size() + annexB.size());
    combined.insert(combined.end(), sequenceHeader_.begin(), sequenceHeader_.end());
    combined.insert(combined.end(), annexB.begin(), annexB.end());
    annexB.swap(combined);
    return true;
}

bool H264Encoder::normalizeAnnexB(const uint8_t* data, size_t size, std::vector<uint8_t>& annexB, bool keyFrame) {
    if (!data || size == 0) return false;

    size_t sc = 0;
    if (IsStartCode(data, size, sc)) {
        annexB.assign(data, data + size);
        if (keyFrame) prependSequenceHeader(annexB);
        return true;
    }

    // Convert length-prefixed NAL units to Annex-B. This also handles common
    // AVCC output from hardware encoders.
    size_t pos = 0;
    while (pos + 4 <= size) {
        uint32_t n = (uint32_t(data[pos]) << 24) | (uint32_t(data[pos+1]) << 16) |
                     (uint32_t(data[pos+2]) << 8) | uint32_t(data[pos+3]);
        pos += 4;
        if (n == 0 || pos + n > size) return false;
        annexB.insert(annexB.end(), {0,0,0,1});
        annexB.insert(annexB.end(), data + pos, data + pos + n);
        pos += n;
    }
    if (keyFrame) prependSequenceHeader(annexB);
    return !annexB.empty();
}

void H264Encoder::requestKeyFrame() {
    forceKeyFrame_ = true;
}

void H264Encoder::shutdown() {
    if (transform_) {
        transform_->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
        transform_->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
    }
    transform_.Reset();
    inputType_.Reset();
    outputType_.Reset();
    codecApi_.Reset();
    converter_.shutdown();
    d3dDevice_.Reset();
    d3dConfigured_ = false;
    dxgiManager_.Reset();
    sequenceHeader_.clear();
    if (started_) MFShutdown();
    started_ = false;
    width_ = height_ = fps_ = bitrateKbps_ = 0;
    forceKeyFrame_ = false;
}

}
