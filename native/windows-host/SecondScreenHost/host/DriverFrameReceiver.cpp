#include "DriverFrameReceiver.h"

#include "HostServer.h"
#include "../../../shared-protocol/FRAME_IPC.h"

#include <dxgi1_6.h>

namespace second_screen {

DriverFrameReceiver::~DriverFrameReceiver() { stop(); }

bool DriverFrameReceiver::start(HostServer* host) {
    if (!host) return false;
    if (running_.exchange(true)) return true;
    host_ = host;
    thread_ = std::make_unique<std::thread>(&DriverFrameReceiver::run, this);
    return true;
}

void DriverFrameReceiver::stop() {
    if (!running_.exchange(false)) return;
    if (readyEvent_) SetEvent(readyEvent_);
    if (thread_ && thread_->joinable()) thread_->join();
    thread_.reset();

    for (auto& m : mutexes_) m.Reset();
    for (auto& t : textures_) t.Reset();
    device1_.Reset();
    context_.Reset();
    device_.Reset();

    if (stateView_) { UnmapViewOfFile(stateView_); stateView_ = nullptr; }
    if (stateMapping_) { CloseHandle(stateMapping_); stateMapping_ = nullptr; }
    if (readyEvent_) { CloseHandle(readyEvent_); readyEvent_ = nullptr; }
    host_ = nullptr;
}

bool DriverFrameReceiver::openSharedState() {
    stateMapping_ = OpenFileMappingW(FILE_MAP_READ, FALSE, frame_ipc::kStateName);
    if (!stateMapping_) return false;

    stateView_ = MapViewOfFile(
        stateMapping_, FILE_MAP_READ, 0, 0, sizeof(frame_ipc::SharedState));
    if (!stateView_) {
        CloseHandle(stateMapping_);
        stateMapping_ = nullptr;
        return false;
    }

    readyEvent_ = OpenEventW(SYNCHRONIZE, FALSE, frame_ipc::kReadyEventName);
    if (!readyEvent_) {
        UnmapViewOfFile(stateView_);
        stateView_ = nullptr;
        CloseHandle(stateMapping_);
        stateMapping_ = nullptr;
        return false;
    }

    return true;
}

bool DriverFrameReceiver::openDevice() {
    auto* state = static_cast<frame_ipc::SharedState*>(stateView_);
    if (state->magic != frame_ipc::kMagic || state->version != frame_ipc::kVersion) {
        return false;
    }

    adapterLuid_ = state->adapterLuid;

    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapterByLuid(adapterLuid_, IID_PPV_ARGS(&adapter)))) return false;

    D3D_FEATURE_LEVEL level{};
    if (FAILED(D3D11CreateDevice(
        adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, &device_, &level, &context_))) {
        return false;
    }

    return SUCCEEDED(device_.As(&device1_));
}

bool DriverFrameReceiver::openTexture(uint32_t slot) {
    if (slot >= 3 || !device1_) return false;
    if (textures_[slot]) return true;

    if (FAILED(device1_->OpenSharedResourceByName(
        frame_ipc::kTextureNames[slot],
        DXGI_SHARED_RESOURCE_READ,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(textures_[slot].GetAddressOf())))) {
        return false;
    }

    return SUCCEEDED(textures_[slot].As(&mutexes_[slot]));
}

void DriverFrameReceiver::run() {
    while (running_) {
        if (!stateView_) {
            if (!openSharedState()) { Sleep(100); continue; }
        }

        auto* state = static_cast<frame_ipc::SharedState*>(stateView_);
        const auto sequence = static_cast<uint64_t>(
            InterlockedCompareExchange64(&state->sequence, 0, 0));

        if (sequence == 0 || sequence == lastSequence_) {
            WaitForSingleObject(readyEvent_, 16);
            continue;
        }

        if (!device_) {
            if (!openDevice()) { Sleep(100); continue; }
        }

        const LONG slot = state->slot;
        const uint32_t width = state->width;
        const uint32_t height = state->height;
        const uint64_t timestampUs = state->timestampUs;
        const auto sequenceAfterSnapshot = static_cast<uint64_t>(
            InterlockedCompareExchange64(&state->sequence, 0, 0));

        if (sequence != sequenceAfterSnapshot) continue;

        if (slot < 0 || slot >= 3 || width == 0 || height == 0) {
            lastSequence_ = sequence;
            continue;
        }

        if (!openTexture(static_cast<uint32_t>(slot))) {
            lastSequence_ = sequence;
            continue;
        }

        if (mutexes_[slot]->AcquireSync(1, 0) != S_OK) continue;

        FrameInfo info{};
        info.width = width;
        info.height = height;
        info.timestampUs = timestampUs;
        info.keyFrame = false;

        host_->submitFrame(textures_[slot].Get(), info);
        mutexes_[slot]->ReleaseSync(0);
        lastSequence_ = sequence;
    }
}

}
