@echo off
REM SecondScreen IddCx Driver MSBuild Automation Script
REM Requires Visual Studio 2022 + Windows Driver Kit (WDK 10/11)

echo ===================================================
echo   SecondScreen Windows Virtual Display Driver Build
echo ===================================================

set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not exist "%VS_DEV_CMD%" (
    set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
)
if not exist "%VS_DEV_CMD%" (
    set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
)

if exist "%VS_DEV_CMD%" (
    call "%VS_DEV_CMD%" -arch=x64
) else (
    echo [WARNING] Visual Studio 2022 Developer Command Prompt not found automatically.
)

echo [*] Building SecondScreenWindows.sln (Driver + DriverInstaller, Release x64)...
msbuild "%~dp0..\SecondScreenWindows.sln" /p:Configuration=Release /p:Platform=x64 /t:Rebuild /m

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] Driver and DriverInstaller built successfully!
    echo Driver output:    %~dp0..\driver\x64\Release\
    echo Installer output: %~dp0..\x64\Release\SecondScreenDriverInstaller.exe
    echo.
    echo Next steps:
    echo   - Dev/test install:  run install_driver.ps1 as Administrator ^(test-signing, self-signed cert^).
    echo   - Signed install:    run SecondScreenDriverInstaller.exe install ^<path-to-signed.inf^> as Administrator.
) else (
    echo.
    echo [ERROR] Build failed with exit code %ERRORLEVEL%.
    echo Please verify Visual Studio 2022 and WDK 10/11 installation.
)
