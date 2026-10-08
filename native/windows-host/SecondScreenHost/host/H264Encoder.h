#pragma once

#include <cstdint>
#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <codecapi.h>
#include <mftransform.h>
#include <mfidl.h>

#include "FrameBridge.h"
#include "GpuVideoConverter.h"

namespace second_screen {

struct EncodedAccessUnit {
    std::vector<uint8_t> annexB;
    uint64_t timestampUs{};
    bool keyFrame{};
};

class H264Encoder {
public:
    ~H264Encoder();

    bool initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrateKbps);
    bool encode(ID3D11Texture2D* d3dTexture, const FrameInfo& info, EncodedAccessUnit& output);
    void requestKeyFrame();
    void shutdown();

private:
    bool createEncoder();
    bool configureEncoder();
    bool configureD3DManager();
    bool setCodecProperty(const GUID& property, VARIANT_BOOL value);
    bool setCodecProperty(const GUID& property, uint32_t value);
    bool drainOutput(EncodedAccessUnit& output, bool& produced);
    bool normalizeAnnexB(const uint8_t* data, size_t size, std::vector<uint8_t>& annexB, bool keyFrame);
    bool prependSequenceHeader(std::vector<uint8_t>& annexB);

    Microsoft::WRL::ComPtr<IMFTransform> transform_;
    Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> dxgiManager_;
    Microsoft::WRL::ComPtr<IMFMediaType> inputType_;
    Microsoft::WRL::ComPtr<IMFMediaType> outputType_;
    Microsoft::WRL::ComPtr<ICodecAPI> codecApi_;
    UINT dxgiManagerResetToken_{};

    uint32_t width_{};
    uint32_t height_{};
    uint32_t fps_{};
    uint32_t bitrateKbps_{};
    bool forceKeyFrame_{false};
    bool started_{false};
    std::vector<uint8_t> sequenceHeader_;
    GpuVideoConverter converter_;
    Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice_;
    bool d3dConfigured_{false};
};

}
