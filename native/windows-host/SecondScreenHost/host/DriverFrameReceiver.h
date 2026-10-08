#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <wrl.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_4.h>

#include "FrameBridge.h"

namespace second_screen {

class HostServer;

class DriverFrameReceiver {
public:
    ~DriverFrameReceiver();

    bool start(HostServer* host);
    void stop();

private:
    void run();
    bool openSharedState();
    bool openDevice();
    bool openTexture(uint32_t slot);

    std::atomic_bool running_{false};
    std::unique_ptr<std::thread> thread_;
    HostServer* host_{nullptr};
    HANDLE stateMapping_{nullptr};
    HANDLE readyEvent_{nullptr};
    void* stateView_{nullptr};
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11Device1> device1_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> textures_[3];
    Microsoft::WRL::ComPtr<IDXGIKeyedMutex> mutexes_[3];
    uint64_t lastSequence_{};
    LUID adapterLuid_{};
};

}
