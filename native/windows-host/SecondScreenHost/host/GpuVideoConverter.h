#pragma once

#include <cstdint>
#include <wrl.h>
#include <d3d11.h>

namespace second_screen {

class GpuVideoConverter {
public:
    bool initialize(ID3D11Device* device, uint32_t width, uint32_t height);
    bool convert(ID3D11Texture2D* source, ID3D11Texture2D** output);
    void shutdown();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11VideoDevice> videoDevice_;
    Microsoft::WRL::ComPtr<ID3D11VideoContext> videoContext_;
    Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator> enumerator_;
    Microsoft::WRL::ComPtr<ID3D11VideoProcessor> processor_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> nv12Texture_;
    Microsoft::WRL::ComPtr<ID3D11VideoProcessorOutputView> outputView_;
    uint32_t width_{};
    uint32_t height_{};
};

}
