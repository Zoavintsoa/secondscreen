// SecondScreen Virtual Display Lifecycle Manager Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "VirtualDisplayManager.h"
#include "../capture/DXGICapture.h"
#include <iostream>

namespace SecondScreen {

    VirtualDisplayManager::VirtualDisplayManager()
        : m_TargetDeviceName(L"\\\\.\\DISPLAY2") {
    }

    VirtualDisplayManager::~VirtualDisplayManager() {
        DeactivateVirtualDisplay();
    }

    bool VirtualDisplayManager::ActivateVirtualDisplay(const MonitorMode& initialMode) {
        std::cout << "[VirtualDisplayManager] Activating Virtual Monitor for iPad secondary display..." << std::endl;

        // 1. Detect primary monitor dimensions to position Display 2 to the right
        auto displays = DXGICaptureManager::EnumerateDisplays();
        int primaryWidth = 1920;
        int primaryHeight = 1080;

        if (!displays.empty()) {
            primaryWidth = displays[0].width;
            primaryHeight = displays[0].height;
        }

        // 2. Position Display 2 as extended desktop adjacent to primary display
        int posX = primaryWidth;
        int posY = 0;
        int width = initialMode.width;
        int height = initialMode.height;

        // Apply display placement and resolution
        DisplayModeManager::PositionDisplay(m_TargetDeviceName, posX, posY, width, height);
        DisplayModeManager::ApplyDisplayMode(m_TargetDeviceName, initialMode);

        RECT bounds = { posX, posY, posX + width, posY + height };
        m_VirtualMonitor.SetBounds(bounds);
        m_VirtualMonitor.SetCurrentMode(initialMode);
        m_VirtualMonitor.SetDeviceName(m_TargetDeviceName);
        m_VirtualMonitor.SetActive(true);

        std::wcout << L"[VirtualDisplayManager] Virtual Monitor ACTIVE on " << m_TargetDeviceName 
                   << L" (Bounds: [" << bounds.left << L"," << bounds.top 
                   << L" -> " << bounds.right << L"," << bounds.bottom << L"])" << std::endl;

        return true;
    }

    bool VirtualDisplayManager::DeactivateVirtualDisplay() {
        if (!m_VirtualMonitor.IsActive()) return true;

        std::cout << "[VirtualDisplayManager] Deactivating Virtual Monitor & restoring desktop topology..." << std::endl;
        DisplayModeManager::DetachDisplay(m_TargetDeviceName);
        DisplayModeManager::ResetDesktopTopology();

        m_VirtualMonitor.SetActive(false);
        return true;
    }

    bool VirtualDisplayManager::SetOrientation(MonitorOrientation orientation) {
        if (!m_VirtualMonitor.IsActive()) return false;

        MonitorMode mode = m_VirtualMonitor.GetCurrentMode();
        if (mode.orientation == orientation) return true;

        // Swap width and height if moving between landscape and portrait
        bool wasLandscape = (mode.orientation == MonitorOrientation::LANDSCAPE || mode.orientation == MonitorOrientation::LANDSCAPE_FLIPPED);
        bool isLandscape = (orientation == MonitorOrientation::LANDSCAPE || orientation == MonitorOrientation::LANDSCAPE_FLIPPED);

        if (wasLandscape != isLandscape) {
            std::swap(mode.width, mode.height);
        }

        mode.orientation = orientation;

        if (DisplayModeManager::ApplyDisplayMode(m_TargetDeviceName, mode)) {
            m_VirtualMonitor.SetCurrentMode(mode);
            RECT curBounds = m_VirtualMonitor.GetBounds();
            curBounds.right = curBounds.left + mode.width;
            curBounds.bottom = curBounds.top + mode.height;
            m_VirtualMonitor.SetBounds(curBounds);
            return true;
        }

        return false;
    }

    bool VirtualDisplayManager::SetResolution(UINT width, UINT height, UINT refreshRate) {
        if (!m_VirtualMonitor.IsActive()) return false;

        MonitorMode mode = m_VirtualMonitor.GetCurrentMode();
        mode.width = width;
        mode.height = height;
        mode.refreshRate = refreshRate;

        if (DisplayModeManager::ApplyDisplayMode(m_TargetDeviceName, mode)) {
            m_VirtualMonitor.SetCurrentMode(mode);
            RECT curBounds = m_VirtualMonitor.GetBounds();
            curBounds.right = curBounds.left + width;
            curBounds.bottom = curBounds.top + height;
            m_VirtualMonitor.SetBounds(curBounds);
            return true;
        }

        return false;
    }

    void VirtualDisplayManager::PrintDisplayStatus() {
        auto displays = DXGICaptureManager::EnumerateDisplays();
        size_t physicalCount = 0;
        size_t virtualCount = 0;

        for (const auto& d : displays) {
            if (d.isSecondScreenVirtualDisplay) {
                virtualCount++;
            } else {
                physicalCount++;
            }
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "SecondScreen Display Status" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Physical Displays: " << physicalCount << std::endl;
        std::cout << "Virtual Displays:  " << virtualCount << std::endl;

        for (size_t i = 0; i < displays.size(); ++i) {
            const auto& d = displays[i];
            std::wcout << L"\nDisplay " << i << L" (" << d.deviceName << L"):" << std::endl;
            std::wcout << L"  Adapter:     " << d.adapterDescription << std::endl;
            std::wcout << L"  Resolution:  " << d.width << L"x" << d.height << std::endl;
            std::wcout << L"  Refresh:     " << (d.refreshRate.Denominator ? (d.refreshRate.Numerator / d.refreshRate.Denominator) : 60) << L" Hz" << std::endl;
            std::wcout << L"  Coordinates: [" << d.desktopCoordinates.left << L"," << d.desktopCoordinates.top 
                       << L" -> " << d.desktopCoordinates.right << L"," << d.desktopCoordinates.bottom << L"]" << std::endl;
            std::wcout << L"  Type:        " << (d.isSecondScreenVirtualDisplay ? L"SecondScreen Virtual Display (ACTIVE)" : L"Physical Hardware Display") << std::endl;
        }

        std::cout << "========================================\n" << std::endl;
    }

}
