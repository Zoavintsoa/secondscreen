// SecondScreen Virtual Display Driver Management Implementation
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "VirtualDisplayDriver.h"
#include <iostream>

namespace SecondScreen {

    VirtualDisplayDriver::VirtualDisplayDriver() {
    }

    VirtualDisplayDriver::~VirtualDisplayDriver() {
    }

    bool VirtualDisplayDriver::IsTestSigningEnabled() {
        // Query system test-signing status from registry or system metrics
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD sysStartOptions = 0;
            DWORD size = sizeof(sysStartOptions);
            DWORD type = 0;
            WCHAR startOptions[512] = {};
            DWORD bufSize = sizeof(startOptions);
            if (RegQueryValueExW(hKey, L"SystemStartOptions", nullptr, &type, reinterpret_cast<LPBYTE>(startOptions), &bufSize) == ERROR_SUCCESS) {
                std::wstring options = startOptions;
                RegCloseKey(hKey);
                return (options.find(L"TESTSIGNING") != std::wstring::npos);
            }
            RegCloseKey(hKey);
        }
        return false;
    }

    DriverStatusInfo VirtualDisplayDriver::QueryStatus() {
        DriverStatusInfo info = {};
        info.isTestModeRequired = true;
        info.isSigned = false; // Self-signed development certificate

        // 1. Check if Driver Service / Device exists in Registry
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\SecondScreenIddCx", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            info.isInstalled = true;
            RegCloseKey(hKey);
        }

        // 2. Enumerate active display devices to check if virtual monitor is present
        DISPLAY_DEVICEW dd = {};
        dd.cb = sizeof(dd);
        for (DWORD i = 0; EnumDisplayDevicesW(nullptr, i, &dd, 0); ++i) {
            std::wstring devString = dd.DeviceString;
            std::wstring devID = dd.DeviceID;

            if (devString.find(L"SecondScreen") != std::wstring::npos ||
                devID.find(L"SecondScreen") != std::wstring::npos ||
                devID.find(L"ROOT\\UNKNOWN") != std::wstring::npos) {
                info.isVirtualMonitorPresent = (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0;
                info.isRunning = true;
                break;
            }
        }

        return info;
    }

    void VirtualDisplayDriver::PrintDriverStatus() {
        DriverStatusInfo info = QueryStatus();
        bool testSigning = IsTestSigningEnabled();

        std::cout << "\n========================================" << std::endl;
        std::cout << "SecondScreen Virtual Display Driver" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Installed:          " << (info.isInstalled ? "YES" : "NO") << std::endl;
        std::cout << "Running:            " << (info.isRunning ? "YES" : "NO") << std::endl;
        std::cout << "Signed:             " << (info.isSigned ? "YES (WHQL / CA)" : "NO (Development / Test-Signed)") << std::endl;
        std::cout << "Test Mode Required: " << (info.isTestModeRequired ? (testSigning ? "YES (Enabled)" : "YES (Disabled - run bcdedit /set testsigning on)") : "NO") << std::endl;
        std::cout << "Virtual Monitor:    " << (info.isVirtualMonitorPresent ? "PRESENT (Attached to Desktop)" : "ABSENT") << std::endl;
        std::cout << "========================================\n" << std::endl;
    }

}
