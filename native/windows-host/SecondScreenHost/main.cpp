#include <windows.h>
#include <objbase.h>
#include <iostream>
#include "host/HostServer.h"

int wmain() {
    const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(comHr) && comHr != RPC_E_CHANGED_MODE) {
        std::wcerr << L"COM initialization failed.\n";
        return 1;
    }

    std::wcout << L"SecondScreen Windows Host\n";
    std::wcout << L"Virtual display -> GPU frame -> H.264 -> LAN stream\n";

    second_screen::HostServer server;
    if (!server.start()) {
        std::wcerr << L"Failed to start host server.\n";
        if (SUCCEEDED(comHr)) CoUninitialize();
        return 1;
    }

    std::wcout << L"Host is ready. Press Enter to stop.\n";
    std::wstring line;
    std::getline(std::wcin, line);

    server.stop();
    if (SUCCEEDED(comHr)) CoUninitialize();
    return 0;
}