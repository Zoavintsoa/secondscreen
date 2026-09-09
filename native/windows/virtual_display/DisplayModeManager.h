// SecondScreen Windows Display Mode & Resolution Manager
// Handles display mode switching, orientation, refresh rate, and desktop topology
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#include "VirtualMonitor.h"
#include <string>
#include <vector>

namespace SecondScreen {

    class DisplayModeManager {
    public:
        DisplayModeManager();
        ~DisplayModeManager();

        // Query all available display modes on a target display device
        static std::vector<MonitorMode> QueryAvailableModes(const std::wstring& deviceName);

        // Apply resolution, refresh rate, and orientation to a target display device
        static bool ApplyDisplayMode(const std::wstring& deviceName, const MonitorMode& mode);

        // Set virtual display position relative to main display (e.g. right, left, top, bottom)
        static bool PositionDisplay(const std::wstring& targetDevice, int posX, int posY, int width, int height);

        // Deactivate / detach a secondary display from the desktop topology
        static bool DetachDisplay(const std::wstring& deviceName);

        // Restore single-monitor primary desktop configuration
        static bool ResetDesktopTopology();
    };

}
