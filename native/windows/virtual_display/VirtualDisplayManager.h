// SecondScreen Virtual Display Lifecycle Manager
// Creates, configures, and deactivates virtual display monitors upon client connect/disconnect
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#include "VirtualMonitor.h"
#include "DisplayModeManager.h"
#include "VirtualDisplayDriver.h"
#include <memory>
#include <string>

namespace SecondScreen {

    class VirtualDisplayManager {
    public:
        VirtualDisplayManager();
        ~VirtualDisplayManager();

        // Activate the virtual secondary monitor upon iPad connection
        bool ActivateVirtualDisplay(const MonitorMode& initialMode);

        // Deactivate and remove the virtual secondary monitor upon iPad disconnection
        bool DeactivateVirtualDisplay();

        // Switch orientation dynamically (Landscape <-> Portrait)
        bool SetOrientation(MonitorOrientation orientation);

        // Switch resolution and refresh rate dynamically
        bool SetResolution(UINT width, UINT height, UINT refreshRate = 60);

        // Check if virtual display is currently attached and active
        bool IsVirtualDisplayActive() const { return m_VirtualMonitor.IsActive(); }

        const VirtualMonitor& GetVirtualMonitor() const { return m_VirtualMonitor; }
        VirtualMonitor& GetVirtualMonitor() { return m_VirtualMonitor; }

        // Formats and prints the display status report (--display-status)
        static void PrintDisplayStatus();

    private:
        VirtualMonitor m_VirtualMonitor;
        std::wstring m_TargetDeviceName;
    };

}
