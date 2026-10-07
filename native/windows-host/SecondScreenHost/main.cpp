#include <windows.h>
#include <iostream>
#include "host/HostServer.h"

int wmain() {
    std::wcout << L"SecondScreen Windows Host\n";
    std::wcout << L"Native host bootstrap. The IddCx driver supplies the virtual display frames.\n";

    second_screen::HostServer server;
    if (!server.start()) {
        std::wcerr << L"Failed to start host server.\n";
        return 1;
    }

    std::wcout << L"Host is ready. Press Enter to stop.\n";
    std::wstring line;
    std::getline(std::wcin, line);

    server.stop();
    return 0;
}
