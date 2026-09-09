// SecondScreen Virtual Monitor Representation
// Encapsulates Display 2 geometry, mode, desktop bounds, and coordinate mapping
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>

namespace SecondScreen {

    enum class MonitorOrientation {
        LANDSCAPE = 0,
        PORTRAIT = 1,
        LANDSCAPE_FLIPPED = 2,
        PORTRAIT_FLIPPED = 3
    };

    struct MonitorMode {
        UINT width;
        UINT height;
        UINT refreshRate;
        MonitorOrientation orientation;
    };

    class VirtualMonitor {
    public:
        VirtualMonitor();
        ~VirtualMonitor();

        void SetDeviceName(const std::wstring& name) { m_DeviceName = name; }
        const std::wstring& GetDeviceName() const { return m_DeviceName; }

        void SetActive(bool active) { m_IsActive = active; }
        bool IsActive() const { return m_IsActive; }

        void SetBounds(const RECT& bounds) { m_DesktopBounds = bounds; }
        const RECT& GetBounds() const { return m_DesktopBounds; }

        UINT GetWidth() const { return m_DesktopBounds.right - m_DesktopBounds.left; }
        UINT GetHeight() const { return m_DesktopBounds.bottom - m_DesktopBounds.top; }

        void SetCurrentMode(const MonitorMode& mode) { m_CurrentMode = mode; }
        const MonitorMode& GetCurrentMode() const { return m_CurrentMode; }

        // Map normalized iPad touch coordinates (0.0f - 1.0f) to Windows Virtual Desktop coordinate space
        POINT NormalizedToVirtualDesktop(float normX, float normY) const;

        // Convert virtual desktop pixel point to Windows SendInput 0-65535 space with MOUSEEVENTF_VIRTUALDESK
        void VirtualDesktopToSendInput(int pixelX, int pixelY, int& outSendX, int& outSendY) const;

        // Supported standard and iPad-specific display modes
        static std::vector<MonitorMode> GetSupportedModes();

    private:
        std::wstring m_DeviceName;
        RECT m_DesktopBounds;
        MonitorMode m_CurrentMode;
        bool m_IsActive;
    };

}
