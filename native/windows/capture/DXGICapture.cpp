// SecondScreen Windows DXGI 1.2 Desktop Duplication GPU Frame Grabber Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "DXGICapture.h"
#include <iostream>
#include <sstream>

namespace SecondScreen {

    namespace {
        bool ContainsSecondScreen(const std::wstring& value) {
            return value.find(L"SecondScreen") != std::wstring::npos;
        }

        void PopulateMonitorIdentity(const std::wstring& displayName, DisplayOutputInfo* info) {
            DISPLAY_DEVICEW displayDevice = {};
            displayDevice.cb = sizeof(displayDevice);
            if (!EnumDisplayDevicesW(displayName.c_str(), 0, &displayDevice, 0)) {
                return;
            }

            info->monitorFriendlyName = displayDevice.DeviceString;
            info->monitorDeviceId = displayDevice.DeviceID;
        }
    }

    DXGICaptureManager::DXGICaptureManager()
        : m_ActiveAdapterIndex(0),
          m_ActiveOutputIndex(0),
          m_FrameCounter(0),
          m_FrameLocked(false),
          m_ClockStart(std::chrono::high_resolution_clock::now()) {
        ZeroMemory(&m_CurrentDisplayInfo, sizeof(m_CurrentDisplayInfo));
    }

    DXGICaptureManager::~DXGICaptureManager() {
        Cleanup();
    }

    std::vector<DisplayOutputInfo> DXGICaptureManager::EnumerateDisplays() {
        std::vector<DisplayOutputInfo> displays;

        ComPtr<IDXGIFactory1> dxgiFactory;
        HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), &dxgiFactory);
        if (FAILED(hr)) return displays;

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT aIdx = 0; dxgiFactory->EnumAdapters1(aIdx, &adapter) != DXGI_ERROR_NOT_FOUND; ++aIdx) {
            DXGI_ADAPTER_DESC1 adapterDesc;
            adapter->GetDesc1(&adapterDesc);

            // Skip Microsoft Basic Render Driver / software adapters if hardware is present
            if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            ComPtr<IDXGIOutput> output;
            for (UINT oIdx = 0; adapter->EnumOutputs(oIdx, &output) != DXGI_ERROR_NOT_FOUND; ++oIdx) {
                DXGI_OUTPUT_DESC outputDesc;
                output->GetDesc(&outputDesc);

                DisplayOutputInfo info = {};
                info.adapterIndex = aIdx;
                info.outputIndex = oIdx;
                info.deviceName = outputDesc.DeviceName;
                info.adapterDescription = adapterDesc.Description;
                PopulateMonitorIdentity(info.deviceName, &info);
                info.desktopCoordinates = outputDesc.DesktopCoordinates;
                info.width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
                info.height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
                info.rotation = outputDesc.Rotation;
                info.isAttachedToDesktop = (outputDesc.AttachedToDesktop != FALSE);

                info.isSecondScreenVirtualDisplay =
                    ContainsSecondScreen(info.monitorFriendlyName) ||
                    ContainsSecondScreen(info.monitorDeviceId) ||
                    ContainsSecondScreen(info.adapterDescription);

                displays.push_back(info);
            }
        }

        return displays;
    }

    bool DXGICaptureManager::CreateD3DDevice(UINT adapterIndex) {
        ComPtr<IDXGIFactory1> dxgiFactory;
        HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), &dxgiFactory);
        if (FAILED(hr)) return false;

        ComPtr<IDXGIAdapter1> adapter;
        hr = dxgiFactory->EnumAdapters1(adapterIndex, &adapter);
        if (FAILED(hr)) return false;

        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1
        };
        D3D_FEATURE_LEVEL featureLevel;

        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        hr = D3D11CreateDevice(
            adapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN, // Required when passing explicit adapter
            nullptr,
            flags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &m_D3DDevice,
            &featureLevel,
            &m_D3DContext
        );

        if (FAILED(hr)) {
            std::wcerr << L"[DXGI Capture] D3D11CreateDevice failed on adapter " << adapterIndex 
                       << L", hr=0x" << std::hex << hr << std::endl;
            return false;
        }

        return true;
    }

    bool DXGICaptureManager::SetupDuplication(UINT adapterIndex, UINT outputIndex) {
        if (!m_D3DDevice) {
            if (!CreateD3DDevice(adapterIndex)) {
                return false;
            }
        }

        ComPtr<IDXGIDevice> dxgiDevice;
        HRESULT hr = m_D3DDevice.As(&dxgiDevice);
        if (FAILED(hr)) return false;

        ComPtr<IDXGIAdapter> dxgiAdapter;
        hr = dxgiDevice->GetAdapter(&dxgiAdapter);
        if (FAILED(hr)) return false;

        ComPtr<IDXGIOutput> dxgiOutput;
        hr = dxgiAdapter->EnumOutputs(outputIndex, &dxgiOutput);
        if (FAILED(hr)) {
            std::wcerr << L"[DXGI Capture] Output " << outputIndex << L" not found on adapter " << adapterIndex << std::endl;
            return false;
        }

        DXGI_OUTPUT_DESC outputDesc;
        dxgiOutput->GetDesc(&outputDesc);

        m_CurrentDisplayInfo.adapterIndex = adapterIndex;
        m_CurrentDisplayInfo.outputIndex = outputIndex;
        m_CurrentDisplayInfo.deviceName = outputDesc.DeviceName;
        PopulateMonitorIdentity(m_CurrentDisplayInfo.deviceName, &m_CurrentDisplayInfo);
        m_CurrentDisplayInfo.desktopCoordinates = outputDesc.DesktopCoordinates;
        m_CurrentDisplayInfo.width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
        m_CurrentDisplayInfo.height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
        m_CurrentDisplayInfo.rotation = outputDesc.Rotation;
        m_CurrentDisplayInfo.isAttachedToDesktop = (outputDesc.AttachedToDesktop != FALSE);
        m_CurrentDisplayInfo.isSecondScreenVirtualDisplay =
            ContainsSecondScreen(m_CurrentDisplayInfo.monitorFriendlyName) ||
            ContainsSecondScreen(m_CurrentDisplayInfo.monitorDeviceId) ||
            ContainsSecondScreen(m_CurrentDisplayInfo.adapterDescription);

        ComPtr<IDXGIOutput1> dxgiOutput1;
        hr = dxgiOutput.As(&dxgiOutput1);
        if (FAILED(hr)) return false;

        // Duplicate output
        hr = dxgiOutput1->DuplicateOutput(m_D3DDevice.Get(), &m_DeskDupl);
        if (FAILED(hr)) {
            std::wcerr << L"[DXGI Capture] DuplicateOutput failed for " << outputDesc.DeviceName 
                       << L", hr=0x" << std::hex << hr << std::endl;
            return false;
        }

        m_ActiveAdapterIndex = adapterIndex;
        m_ActiveOutputIndex = outputIndex;

        std::wcout << L"[DXGI Capture] Attached to Display " << outputIndex 
                   << L" (" << outputDesc.DeviceName << L" - " << m_CurrentDisplayInfo.width 
                   << L"x" << m_CurrentDisplayInfo.height << L")" << std::endl;

        return true;
    }

    bool DXGICaptureManager::Initialize(UINT targetDisplayIndex) {
        Cleanup();
        auto displays = EnumerateDisplays();
        if (targetDisplayIndex >= displays.size()) {
            std::wcerr << L"[DXGI Capture] Target display index " << targetDisplayIndex 
                       << L" out of range. Discovered displays: " << displays.size() << std::endl;
            return false;
        }

        auto& target = displays[targetDisplayIndex];
        return SetupDuplication(target.adapterIndex, target.outputIndex);
    }

    bool DXGICaptureManager::InitializeByDisplayName(const std::wstring& targetDisplayName) {
        Cleanup();
        auto displays = EnumerateDisplays();
        for (const auto& disp : displays) {
            if (disp.deviceName.find(targetDisplayName) != std::wstring::npos ||
                disp.monitorFriendlyName.find(targetDisplayName) != std::wstring::npos ||
                disp.monitorDeviceId.find(targetDisplayName) != std::wstring::npos) {
                return SetupDuplication(disp.adapterIndex, disp.outputIndex);
            }
        }

        std::wcerr << L"[DXGI Capture] Display with name matching '" << targetDisplayName << L"' not found." << std::endl;
        return false;
    }

    bool DXGICaptureManager::InitializeSecondScreenVirtualDisplay() {
        Cleanup();
        auto displays = EnumerateDisplays();
        std::vector<DisplayOutputInfo> candidates;
        for (const auto& display : displays) {
            if (display.isAttachedToDesktop && display.isSecondScreenVirtualDisplay) {
                candidates.push_back(display);
            }
        }

        if (candidates.size() != 1) {
            std::wcerr << L"[DXGI Capture] Expected exactly one attached SecondScreen virtual display; found "
                       << candidates.size() << L"." << std::endl;
            return false;
        }

        return SetupDuplication(candidates.front().adapterIndex, candidates.front().outputIndex);
    }

    bool DXGICaptureManager::AcquireNextFrame(CapturedGPUFrame* pOutFrame, UINT timeoutMs) {
        if (!m_DeskDupl || !pOutFrame) return false;

        if (m_FrameLocked) {
            ReleaseFrame();
        }

        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        ComPtr<IDXGIResource> desktopResource;
        HRESULT hr = m_DeskDupl->AcquireNextFrame(timeoutMs, &frameInfo, &desktopResource);

        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            // No new dirty region or update during timeout window
            return false;
        }

        if (FAILED(hr)) {
            if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
                std::wcerr << L"[DXGI Capture] Access lost or device reset (0x" << std::hex << hr << L"), attempting recovery..." << std::endl;
                Reinitialize();
            }
            return false;
        }

        m_FrameLocked = true;

        ComPtr<ID3D11Texture2D> acquiredTexture;
        hr = desktopResource.As(&acquiredTexture);
        if (FAILED(hr)) {
            ReleaseFrame();
            return false;
        }

        D3D11_TEXTURE2D_DESC desc;
        acquiredTexture->GetDesc(&desc);

        // Ensure internal GPU frame texture matches incoming frame properties
        if (!m_GpuFrameTexture) {
            D3D11_TEXTURE2D_DESC texDesc = desc;
            texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            texDesc.MiscFlags = 0;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.CPUAccessFlags = 0;

            hr = m_D3DDevice->CreateTexture2D(&texDesc, nullptr, &m_GpuFrameTexture);
            if (FAILED(hr)) {
                ReleaseFrame();
                return false;
            }
        } else {
            D3D11_TEXTURE2D_DESC curDesc;
            m_GpuFrameTexture->GetDesc(&curDesc);
            if (curDesc.Width != desc.Width || curDesc.Height != desc.Height || curDesc.Format != desc.Format) {
                m_GpuFrameTexture.Reset();
                D3D11_TEXTURE2D_DESC texDesc = desc;
                texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                texDesc.MiscFlags = 0;
                texDesc.Usage = D3D11_USAGE_DEFAULT;
                texDesc.CPUAccessFlags = 0;

                hr = m_D3DDevice->CreateTexture2D(&texDesc, nullptr, &m_GpuFrameTexture);
                if (FAILED(hr)) {
                    ReleaseFrame();
                    return false;
                }
            }
        }

        // Fast GPU VRAM-to-VRAM copy (< 0.1ms)
        m_D3DContext->CopyResource(m_GpuFrameTexture.Get(), acquiredTexture.Get());

        // Immediately release Desktop Duplication lock to eliminate frame starvation
        ReleaseFrame();

        auto now = std::chrono::high_resolution_clock::now();
        uint64_t timestampUs = std::chrono::duration_cast<std::chrono::microseconds>(now - m_ClockStart).count();

        pOutFrame->texture = m_GpuFrameTexture;
        pOutFrame->textureDesc = desc;
        pOutFrame->width = desc.Width;
        pOutFrame->height = desc.Height;
        pOutFrame->format = desc.Format;
        pOutFrame->captureTimestampUs = timestampUs;
        pOutFrame->frameSequenceNumber = ++m_FrameCounter;
        pOutFrame->isKeyFrameRequired = (frameInfo.TotalMetadataBufferSize > 0);

        return true;
    }

    void DXGICaptureManager::ReleaseFrame() {
        if (m_DeskDupl && m_FrameLocked) {
            m_DeskDupl->ReleaseFrame();
            m_FrameLocked = false;
        }
    }

    bool DXGICaptureManager::Reinitialize() {
        Cleanup();
        Sleep(100); // Allow OS desktop mode transition
        return SetupDuplication(m_ActiveAdapterIndex, m_ActiveOutputIndex);
    }

    void DXGICaptureManager::Cleanup() {
        ReleaseFrame();
        m_GpuFrameTexture.Reset();
        m_DeskDupl.Reset();
        m_D3DContext.Reset();
        m_D3DDevice.Reset();
        m_FrameCounter = 0;
    }

}
