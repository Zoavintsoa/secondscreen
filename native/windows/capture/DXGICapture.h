// SecondScreen Windows DXGI 1.2 Desktop Duplication GPU Frame Grabber
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

using Microsoft::WRL::ComPtr;

namespace SecondScreen {

    struct DisplayOutputInfo {
        UINT adapterIndex;
        UINT outputIndex;
        std::wstring deviceName;
        std::wstring adapterDescription;
        std::wstring monitorFriendlyName;
        std::wstring monitorDeviceId;
        RECT desktopCoordinates;
        UINT width;
        UINT height;
        DXGI_RATIONAL refreshRate;
        DXGI_MODE_ROTATION rotation;
        bool isAttachedToDesktop;
        bool isSecondScreenVirtualDisplay;
    };

    struct CapturedGPUFrame {
        ComPtr<ID3D11Texture2D> texture;
        D3D11_TEXTURE2D_DESC textureDesc;
        UINT width;
        UINT height;
        DXGI_FORMAT format;
        uint64_t captureTimestampUs;
        uint64_t frameSequenceNumber;
        bool isKeyFrameRequired;
        RECT dirtyRect;
    };

    class DXGICaptureManager {
    public:
        DXGICaptureManager();
        ~DXGICaptureManager();

        // Enumerate all active and virtual displays across all GPU adapters
        static std::vector<DisplayOutputInfo> EnumerateDisplays();

        // Initialize capture on a specific display index or by name
        bool Initialize(UINT targetDisplayIndex = 0);
        bool InitializeByDisplayName(const std::wstring& targetDisplayName);
        bool InitializeSecondScreenVirtualDisplay();

        // Acquire next GPU frame directly in VRAM (zero-copy)
        bool AcquireNextFrame(CapturedGPUFrame* pOutFrame, UINT timeoutMs = 16);

        // Explicitly release acquired frame lock
        void ReleaseFrame();

        // Check and handle GPU device removal or desktop switch
        bool Reinitialize();

        // Cleanup all DirectX resources
        void Cleanup();

        bool IsCapturing() const { return m_DeskDupl != nullptr; }
        const DisplayOutputInfo& GetCurrentDisplayInfo() const { return m_CurrentDisplayInfo; }
        ID3D11Device* GetDevice() const { return m_D3DDevice.Get(); }
        ID3D11DeviceContext* GetContext() const { return m_D3DContext.Get(); }

    private:
        bool CreateD3DDevice(UINT adapterIndex);
        bool SetupDuplication(UINT adapterIndex, UINT outputIndex);

        ComPtr<ID3D11Device> m_D3DDevice;
        ComPtr<ID3D11DeviceContext> m_D3DContext;
        ComPtr<IDXGIOutputDuplication> m_DeskDupl;
        ComPtr<ID3D11Texture2D> m_GpuFrameTexture;
        DisplayOutputInfo m_CurrentDisplayInfo;

        UINT m_ActiveAdapterIndex;
        UINT m_ActiveOutputIndex;
        uint64_t m_FrameCounter;
        bool m_FrameLocked;
        std::chrono::high_resolution_clock::time_point m_ClockStart;
    };

}
