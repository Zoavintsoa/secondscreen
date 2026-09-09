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

echo [*] Building SecondScreenIddCx.vcxproj (Release x64)...
msbuild "%~dp0..\driver\SecondScreenIddCx.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Rebuild

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] Driver binary built successfully!
    echo Output directory: %~dp0..\driver\x64\Release\
    echo.
    echo NOTE: SecondScreenHost.exe and SecondScreenDriverInstaller.exe are now
    echo built separately via CMake from the repository root:
    echo   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    echo   cmake --build build --config Release
    echo.
    echo Next step: Run install_driver.ps1 as Administrator to install Display 2.
) else (
    echo.
    echo [ERROR] Build failed with exit code %ERRORLEVEL%.
    echo Please verify Visual Studio 2022 and WDK 10/11 installation.
)
