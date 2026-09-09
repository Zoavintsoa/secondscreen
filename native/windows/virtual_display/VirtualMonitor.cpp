// SecondScreen Virtual Monitor Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "VirtualMonitor.h"
#include <algorithm>

namespace SecondScreen {

    VirtualMonitor::VirtualMonitor()
        : m_DeviceName(L"\\\\.\\DISPLAY2"),
          m_DesktopBounds({ 1920, 0, 3840, 1080 }),
          m_CurrentMode({ 1920, 1080, 60, MonitorOrientation::LANDSCAPE }),
          m_IsActive(false) {
    }

    VirtualMonitor::~VirtualMonitor() {
    }

    POINT VirtualMonitor::NormalizedToVirtualDesktop(float normX, float normY) const {
        float clampedX = std::max(0.0f, std::min(1.0f, normX));
        float clampedY = std::max(0.0f, std::min(1.0f, normY));

        LONG width = m_DesktopBounds.right - m_DesktopBounds.left;
        LONG height = m_DesktopBounds.bottom - m_DesktopBounds.top;

        POINT pt;
        pt.x = m_DesktopBounds.left + static_cast<LONG>(clampedX * width);
        pt.y = m_DesktopBounds.top + static_cast<LONG>(clampedY * height);
        return pt;
    }

    void VirtualMonitor::VirtualDesktopToSendInput(int pixelX, int pixelY, int& outSendX, int& outSendY) const {
        int vLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int vTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int vWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int vHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        if (vWidth <= 0) vWidth = 1920;
        if (vHeight <= 0) vHeight = 1080;

        outSendX = static_cast<int>(((double)(pixelX - vLeft) / (double)vWidth) * 65535.0);
        outSendY = static_cast<int>(((double)(pixelY - vTop) / (double)vHeight) * 65535.0);
    }

    std::vector<MonitorMode> VirtualMonitor::GetSupportedModes() {
        return {
            // Standard 16:9 Desktop Resolutions
            { 1920, 1080, 60, MonitorOrientation::LANDSCAPE },
            { 1920, 1080, 30, MonitorOrientation::LANDSCAPE },
            { 2560, 1440, 60, MonitorOrientation::LANDSCAPE },
            { 1280, 720,  60, MonitorOrientation::LANDSCAPE },
            { 1280, 720,  30, MonitorOrientation::LANDSCAPE },

            // Portrait Orientations
            { 1080, 1920, 60, MonitorOrientation::PORTRAIT },
            { 1080, 1920, 30, MonitorOrientation::PORTRAIT },
            { 720,  1280, 60, MonitorOrientation::PORTRAIT },

            // iPad Native Aspect Ratio Profiles (4:3 & 3:2 High-DPI Workspace)
            { 2048, 1536, 60, MonitorOrientation::LANDSCAPE }, // iPad Pro 9.7 / Air
            { 2388, 1668, 60, MonitorOrientation::LANDSCAPE }, // iPad Pro 11-inch
            { 2732, 2048, 60, MonitorOrientation::LANDSCAPE }, // iPad Pro 12.9-inch
            { 1536, 2048, 60, MonitorOrientation::PORTRAIT },
            { 1668, 2388, 60, MonitorOrientation::PORTRAIT }
        };
    }

}
