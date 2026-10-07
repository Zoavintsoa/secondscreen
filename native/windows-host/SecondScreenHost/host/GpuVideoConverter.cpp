#include "GpuVideoConverter.h"

#include <algorithm>

namespace second_screen {

bool GpuVideoConverter::initialize(ID3D11Device* device, uint32_t width, uint32_t height) {
    shutdown();
    if (!device || !width || !height) return false;

    device_ = device;
    device_->GetImmediateContext(&context_);
    if (!context_) return false;

    if (FAILED(device_->QueryInterface(IID_PPV_ARGS(&videoDevice_)))) return false;
    if (FAILED(context_->QueryInterface(IID_PPV_ARGS(&videoContext_)))) return false;

    D3D11_VIDEO_PROCESSOR_CONTENT_DESC desc{};
    desc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
    desc.InputFrameRate.Numerator = 60;
    desc.InputFrameRate.Denominator = 1;
    desc.InputWidth = width;
    desc.InputHeight = height;
    desc.OutputFrameRate.Numerator = 60;
    desc.OutputFrameRate.Denominator = 1;
    desc.OutputWidth = width;
    desc.OutputHeight = height;
    desc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

    if (FAILED(videoDevice_->CreateVideoProcessorEnumerator(&desc, &enumerator_))) return false;

    UINT support = 0;
    if (FAILED(enumerator_->CheckVideoProcessorFormat(DXGI_FORMAT_B8G8R8A8_UNORM, &support)) ||
        !(support & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT)) return false;
    if (FAILED(enumerator_->CheckVideoProcessorFormat(DXGI_FORMAT_NV12, &support)) ||
        !(support & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT)) return false;

    if (FAILED(videoDevice_->CreateVideoProcessor(enumerator_.Get(), 0, &processor_))) return false;

    D3D11_TEXTURE2D_DESC outDesc{};
    outDesc.Width = width;
    outDesc.Height = height;
    outDesc.MipLevels = 1;
    outDesc.ArraySize = 1;
    outDesc.Format = DXGI_FORMAT_NV12;
    outDesc.SampleDesc.Count = 1;
    outDesc.Usage = D3D11_USAGE_DEFAULT;
    outDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(device_->CreateTexture2D(&outDesc, nullptr, &nv12Texture_))) return false;

    D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outView{};
    outView.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
    outView.Texture2D.MipSlice = 0;

    if (FAILED(videoDevice_->CreateVideoProcessorOutputView(
        nv12Texture_.Get(), enumerator_.Get(), &outView, &outputView_))) return false;

    width_ = width;
    height_ = height;
    return true;
}

bool GpuVideoConverter::convert(ID3D11Texture2D* source, ID3D11Texture2D** output) {
    if (!source || !output || !processor_ || !outputView_) return false;

    D3D11_TEXTURE2D_DESC srcDesc{};
    source->GetDesc(&srcDesc);
    if (srcDesc.Width != width_ || srcDesc.Height != height_) return false;

    Microsoft::WRL::ComPtr<ID3D11VideoProcessorInputView> inputView;
    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inView{};
    inView.FourCC = 0;
    inView.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
    inView.Texture2D.MipSlice = 0;
    inView.Texture2D.ArraySlice = 0;

    if (FAILED(videoDevice_->CreateVideoProcessorInputView(
        source, enumerator_.Get(), &inView, &inputView))) return false;

    RECT full{};
    full.right = static_cast<LONG>(width_);
    full.bottom = static_cast<LONG>(height_);

    videoContext_->VideoProcessorSetStreamFrameFormat(
        processor_.Get(), 0, D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);
    videoContext_->VideoProcessorSetStreamSourceRect(
        processor_.Get(), 0, TRUE, &full);
    videoContext_->VideoProcessorSetStreamDestRect(
        processor_.Get(), 0, TRUE, &full);
    videoContext_->VideoProcessorSetOutputTargetRect(
        processor_.Get(), TRUE, &full);

    D3D11_VIDEO_PROCESSOR_STREAM stream{};
    stream.Enable = TRUE;
    stream.OutputIndex = 0;
    stream.InputFrameOrField = 0;
    stream.PastFrames = 0;
    stream.FutureFrames = 0;
    stream.pInputSurface = inputView.Get();

    if (FAILED(videoContext_->VideoProcessorBlt(
        processor_.Get(), outputView_.Get(), 0, 1, &stream))) return false;

    context_->Flush();
    *output = nv12Texture_.Get();
    (*output)->AddRef();
    return true;
}

void GpuVideoConverter::shutdown() {
    outputView_.Reset();
    nv12Texture_.Reset();
    processor_.Reset();
    enumerator_.Reset();
    videoContext_.Reset();
    videoDevice_.Reset();
    context_.Reset();
    device_.Reset();
    width_ = height_ = 0;
}

}
