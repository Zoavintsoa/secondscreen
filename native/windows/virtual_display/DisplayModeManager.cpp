// SecondScreen Windows Display Mode & Resolution Manager Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "DisplayModeManager.h"
#include <iostream>

namespace SecondScreen {

    DisplayModeManager::DisplayModeManager() {
    }

    DisplayModeManager::~DisplayModeManager() {
    }

    std::vector<MonitorMode> DisplayModeManager::QueryAvailableModes(const std::wstring& deviceName) {
        std::vector<MonitorMode> modes;

        DEVMODEW dm = {};
        dm.dmSize = sizeof(dm);

        for (DWORD i = 0; EnumDisplaySettingsW(deviceName.c_str(), i, &dm); ++i) {
            MonitorMode mode = {};
            mode.width = dm.dmPelsWidth;
            mode.height = dm.dmPelsHeight;
            mode.refreshRate = dm.dmDisplayFrequency;
            mode.orientation = static_cast<MonitorOrientation>(dm.dmDisplayOrientation);

            // Avoid duplicate mode entries
            bool exists = false;
            for (const auto& m : modes) {
                if (m.width == mode.width && m.height == mode.height && 
                    m.refreshRate == mode.refreshRate && m.orientation == mode.orientation) {
                    exists = true;
                    break;
                }
            }

            if (!exists && mode.width >= 640 && mode.height >= 480) {
                modes.push_back(mode);
            }
        }

        return modes;
    }

    bool DisplayModeManager::ApplyDisplayMode(const std::wstring& deviceName, const MonitorMode& mode) {
        DEVMODEW dm = {};
        dm.dmSize = sizeof(dm);
        dm.dmPelsWidth = mode.width;
        dm.dmPelsHeight = mode.height;
        dm.dmDisplayFrequency = mode.refreshRate;
        dm.dmDisplayOrientation = static_cast<DWORD>(mode.orientation);
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYORIENTATION;

        LONG res = ChangeDisplaySettingsExW(
            deviceName.c_str(),
            &dm,
            nullptr,
            CDS_UPDATEREGISTRY | CDS_GLOBAL,
            nullptr
        );

        if (res == DISP_CHANGE_SUCCESSFUL) {
            std::wcout << L"[DisplayModeManager] Applied mode on " << deviceName 
                       << L": " << mode.width << L"x" << mode.height 
                       << L" @" << mode.refreshRate << L"Hz" << std::endl;
            return true;
        }

        std::wcerr << L"[DisplayModeManager] Failed to apply display mode on " << deviceName 
                   << L", error code: " << res << std::endl;
        return false;
    }

    bool DisplayModeManager::PositionDisplay(const std::wstring& targetDevice, int posX, int posY, int width, int height) {
        DEVMODEW dm = {};
        dm.dmSize = sizeof(dm);
        dm.dmPosition.x = posX;
        dm.dmPosition.y = posY;
        dm.dmPelsWidth = width;
        dm.dmPelsHeight = height;
        dm.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;

        LONG res = ChangeDisplaySettingsExW(
            targetDevice.c_str(),
            &dm,
            nullptr,
            CDS_UPDATEREGISTRY | CDS_NORESET,
            nullptr
        );

        if (res != DISP_CHANGE_SUCCESSFUL) {
            return false;
        }

        // Apply topological layout changes
        ChangeDisplaySettingsExW(nullptr, nullptr, nullptr, 0, nullptr);
        return true;
    }

    bool DisplayModeManager::DetachDisplay(const std::wstring& deviceName) {
        DEVMODEW dm = {};
        dm.dmSize = sizeof(dm);
        dm.dmPelsWidth = 0;
        dm.dmPelsHeight = 0;
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION;

        LONG res = ChangeDisplaySettingsExW(
            deviceName.c_str(),
            &dm,
            nullptr,
            CDS_UPDATEREGISTRY | CDS_GLOBAL,
            nullptr
        );

        return (res == DISP_CHANGE_SUCCESSFUL);
    }

    bool DisplayModeManager::ResetDesktopTopology() {
        LONG res = ChangeDisplaySettingsExW(nullptr, nullptr, nullptr, 0, nullptr);
        return (res == DISP_CHANGE_SUCCESSFUL);
    }

}
