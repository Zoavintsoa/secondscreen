#pragma once

#include <cstdint>
#include <windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <dxgi1_2.h>

namespace second_screen {

class DriverFramePublisher {
public:
    ~DriverFramePublisher();

    bool initialize(ID3D11Device* device, uint32_t width, uint32_t height);
    void shutdown();
    bool publish(ID3D11Texture2D* source, uint64_t timestampUs);

private:
    struct Slot {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<IDXGIKeyedMutex> mutex;
        HANDLE sharedHandle{nullptr};
    };

    bool createSharedObjects();
    bool createSlots();
    void closeHandles();

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Slot slots_[3];
    HANDLE stateMapping_{nullptr};
    HANDLE readyEvent_{nullptr};
    void* stateView_{nullptr};
    uint32_t width_{};
    uint32_t height_{};
    LUID adapterLuid_{};
    LARGE_INTEGER qpcFrequency_{};
    LONG64 sequence_{};
    uint32_t nextSlot_{};
};

}
