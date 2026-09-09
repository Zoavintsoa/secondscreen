// SecondScreen Virtual Display Driver Management
// Driver status, test-signing detection, and installation verification
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace SecondScreen {

    struct DriverStatusInfo {
        bool isInstalled = false;
        bool isRunning = false;
        bool isSigned = false;
        bool isTestModeRequired = true;
        bool isVirtualMonitorPresent = false;
        std::wstring driverVersion;
        std::wstring infPath;
    };

    class VirtualDisplayDriver {
    public:
        VirtualDisplayDriver();
        ~VirtualDisplayDriver();

        // Query status of the SecondScreen IddCx driver on the current Windows machine
        static DriverStatusInfo QueryStatus();

        // Format and print the driver status report (--driver-status)
        static void PrintDriverStatus();

        // Check if Windows is currently in Test Signing mode (BCDEdit testsigning on)
        static bool IsTestSigningEnabled();
    };

}
