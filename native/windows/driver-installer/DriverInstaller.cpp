#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <devguid.h>
#include <newdev.h>
#include <setupapi.h>

#include <iostream>
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#pragma comment(lib, "newdev.lib")
#pragma comment(lib, "setupapi.lib")

namespace {
constexpr wchar_t kHardwareId[] = L"Root\\SecondScreenVirtualDisplay";

bool IsAdministrator() {
    BOOL isMember = FALSE;
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID administratorsGroup = nullptr;
    if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0, &administratorsGroup)) {
        return false;
    }
    const BOOL result = CheckTokenMembership(nullptr, administratorsGroup, &isMember);
    FreeSid(administratorsGroup);
    return result && isMember;
}

bool CreateRootDevice() {
    HDEVINFO deviceInfoSet = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, nullptr);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return false;
    }

    SP_DEVINFO_DATA deviceInfo = {};
    deviceInfo.cbSize = sizeof(deviceInfo);
    const BOOL created = SetupDiCreateDeviceInfoW(deviceInfoSet, kHardwareId, &GUID_DEVCLASS_DISPLAY,
                                                   L"SecondScreen Virtual Display", nullptr,
                                                   DICD_GENERATE_ID, &deviceInfo);
    if (!created) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return false;
    }

    std::vector<wchar_t> hardwareId(std::size(kHardwareId) + 1, L'\0');
    std::copy(std::begin(kHardwareId), std::end(kHardwareId), hardwareId.begin());
    const BOOL hardwareIdSet = SetupDiSetDeviceRegistryPropertyW(
        deviceInfoSet,
        &deviceInfo,
        SPDRP_HARDWAREID,
        reinterpret_cast<const BYTE*>(hardwareId.data()),
        static_cast<DWORD>(hardwareId.size() * sizeof(wchar_t)));
    const BOOL registered = hardwareIdSet && SetupDiCallClassInstaller(DIF_REGISTERDEVICE, deviceInfoSet, &deviceInfo);
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return registered == TRUE;
}

bool InstallDriver(const std::wstring& infPath) {
    if (!CreateRootDevice()) {
        std::wcerr << L"Failed to create the root-enumerated SecondScreen device. Win32 error: " << GetLastError() << std::endl;
        return false;
    }

    BOOL rebootRequired = FALSE;
    if (!UpdateDriverForPlugAndPlayDevicesW(nullptr, kHardwareId, infPath.c_str(), INSTALLFLAG_FORCE, &rebootRequired)) {
        std::wcerr << L"Failed to install the signed driver package. Win32 error: " << GetLastError() << std::endl;
        return false;
    }

    std::wcout << (rebootRequired ? L"REBOOT_REQUIRED" : L"INSTALLED") << std::endl;
    return true;
}

bool RemoveDevices() {
    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, kHardwareId, nullptr, DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool removed = false;
    for (DWORD index = 0;; ++index) {
        SP_DEVINFO_DATA deviceInfo = {};
        deviceInfo.cbSize = sizeof(deviceInfo);
        if (!SetupDiEnumDeviceInfo(deviceInfoSet, index, &deviceInfo)) {
            break;
        }
        DWORD requiredSize = 0;
        SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, &deviceInfo, SPDRP_HARDWAREID, nullptr, nullptr, 0, &requiredSize);
        if (requiredSize == 0) {
            continue;
        }
        std::vector<wchar_t> hardwareIds(requiredSize / sizeof(wchar_t), L'\0');
        if (!SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, &deviceInfo, SPDRP_HARDWAREID, nullptr,
                                               reinterpret_cast<BYTE*>(hardwareIds.data()), requiredSize, nullptr) ||
            std::wstring(hardwareIds.data()).find(kHardwareId) == std::wstring::npos) {
            continue;
        }
        SP_REMOVEDEVICE_PARAMS removeParams = {};
        removeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        removeParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
        removeParams.Scope = DI_REMOVEDEVICE_GLOBAL;
        removeParams.HwProfile = 0;
        if (SetupDiSetClassInstallParamsW(deviceInfoSet, &deviceInfo, &removeParams.ClassInstallHeader, sizeof(removeParams)) &&
            SetupDiCallClassInstaller(DIF_REMOVE, deviceInfoSet, &deviceInfo)) {
            removed = true;
        }
    }
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return removed;
}
}

int wmain(int argc, wchar_t* argv[]) {
    if (!IsAdministrator()) {
        std::wcerr << L"Administrator privileges are required." << std::endl;
        return ERROR_ACCESS_DENIED;
    }
    if (argc == 3 && std::wstring(argv[1]) == L"install") {
        return InstallDriver(argv[2]) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
    }
    if (argc == 2 && std::wstring(argv[1]) == L"uninstall") {
        return RemoveDevices() ? ERROR_SUCCESS : ERROR_NOT_FOUND;
    }
    std::wcerr << L"Usage: SecondScreenDriverInstaller install <driver.inf> | uninstall" << std::endl;
    return ERROR_INVALID_PARAMETER;
}
