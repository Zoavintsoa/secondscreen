#include "DriverFramePublisher.h"

#include "../../../shared-protocol/FRAME_IPC.h"

#include <sddl.h>

#pragma comment(lib, "Advapi32.lib")

namespace second_screen {

namespace {
SECURITY_ATTRIBUTES SharedSecurityAttributes(PSECURITY_DESCRIPTOR* descriptor) {
    *descriptor = nullptr;
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, FALSE};
    const wchar_t* dacl = L"D:(A;;GA;;;LS)(A;;GA;;;BA)(A;;GA;;;IU)";
    if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
            dacl, SDDL_REVISION_1, descriptor, nullptr)) {
        sa.lpSecurityDescriptor = *descriptor;
    }
    return sa;
}
}

DriverFramePublisher::~DriverFramePublisher() {
    shutdown();
}

bool DriverFramePublisher::initialize(ID3D11Device* device, uint32_t width, uint32_t height) {
    shutdown();
    if (!device || !width || !height) return false;

    device_ = device;
    device_->GetImmediateContext(&context_);
    width_ = width;
    height_ = height;

    if (!createSharedObjects() || !createSlots()) {
        shutdown();
        return false;
    }

    sequence_ = 0;
    nextSlot_ = 0;
    return true;
}

bool DriverFramePublisher::createSharedObjects() {
    PSECURITY_DESCRIPTOR sd = nullptr;
    SECURITY_ATTRIBUTES sa = SharedSecurityAttributes(&sd);

    stateMapping_ = CreateFileMappingW(
        INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0,
        static_cast<DWORD>(sizeof(frame_ipc::SharedState)),
        frame_ipc::kStateName);
    if (sd) LocalFree(sd);
    if (!stateMapping_) return false;

    stateView_ = MapViewOfFile(
        stateMapping_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0,
        sizeof(frame_ipc::SharedState));
    if (!stateView_) return false;

    auto* state = static_cast<frame_ipc::SharedState*>(stateView_);
    ZeroMemory(state, sizeof(*state));
    state->magic = frame_ipc::kMagic;
    state->version = frame_ipc::kVersion;
    state->slot = -1;

    sd = nullptr;
    sa = SharedSecurityAttributes(&sd);
    readyEvent_ = CreateEventW(&sa, FALSE, FALSE, frame_ipc::kReadyEventName);
    if (sd) LocalFree(sd);
    return readyEvent_ != nullptr;
}

bool DriverFramePublisher::createSlots() {
    for (uint32_t i = 0; i < 3; ++i) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width_;
        desc.Height = height_;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE |
                         D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

        if (FAILED(device_->CreateTexture2D(&desc, nullptr, &slots_[i].texture)))
            return false;

        if (FAILED(slots_[i].texture.As(&slots_[i].mutex)))
            return false;

        Microsoft::WRL::ComPtr<IDXGIResource1> resource;
        if (FAILED(slots_[i].texture.As(&resource)))
            return false;

        PSECURITY_DESCRIPTOR sd = nullptr;
        SECURITY_ATTRIBUTES sa = SharedSecurityAttributes(&sd);
        const HRESULT hr = resource->CreateSharedHandle(
            &sa,
            DXGI_SHARED_RESOURCE_READ,
            frame_ipc::kTextureNames[i],
            &slots_[i].sharedHandle);
        if (sd) LocalFree(sd);
        if (FAILED(hr)) return false;
    }
    return true;
}

bool DriverFramePublisher::publish(ID3D11Texture2D* source, uint64_t timestampUs) {
    if (!source || !context_ || !stateView_) return false;

    for (uint32_t attempt = 0; attempt < 3; ++attempt) {
        const uint32_t index = (nextSlot_ + attempt) % 3;
        if (slots_[index].mutex->AcquireSync(0, 0) != S_OK) continue;

        context_->CopyResource(slots_[index].texture.Get(), source);
        context_->Flush();

        if (slots_[index].mutex->ReleaseSync(1) != S_OK) continue;

        auto* state = static_cast<frame_ipc::SharedState*>(stateView_);
        state->width = width_;
        state->height = height_;
        state->timestampUs = timestampUs;
        LUID luid{};
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        if (SUCCEEDED(device_.As(&dxgiDevice)) &&
            SUCCEEDED(dxgiDevice->GetAdapter(&adapter))) {
            DXGI_ADAPTER_DESC desc{};
            if (SUCCEEDED(adapter->GetDesc(&desc))) luid = desc.AdapterLuid;
        }
        state->adapterLuid = luid;

        state->slot = static_cast<LONG>(index);
        InterlockedExchange64(&state->sequence, ++sequence_);
        SetEvent(readyEvent_);

        nextSlot_ = (index + 1) % 3;
        return true;
    }
    return false;
}

void DriverFramePublisher::closeHandles() {
    for (auto& slot : slots_) {
        if (slot.sharedHandle) {
            CloseHandle(slot.sharedHandle);
            slot.sharedHandle = nullptr;
        }
    }
}

void DriverFramePublisher::shutdown() {
    closeHandles();
    for (auto& slot : slots_) {
        slot.mutex.Reset();
        slot.texture.Reset();
    }

    if (stateView_) {
        UnmapViewOfFile(stateView_);
        stateView_ = nullptr;
    }
    if (stateMapping_) {
        CloseHandle(stateMapping_);
        stateMapping_ = nullptr;
    }
    if (readyEvent_) {
        CloseHandle(readyEvent_);
        readyEvent_ = nullptr;
    }

    context_.Reset();
    device_.Reset();
    width_ = height_ = 0;
    sequence_ = 0;
    nextSlot_ = 0;
}

}
