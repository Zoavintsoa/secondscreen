# SecondScreen - Windows Host Native Driver & Pipeline Documentation

## 1. Overview
The Windows Host component implements a genuine User-Mode Driver Framework (UMDF 2.0) Indirect Display Driver (`IddCx`) that creates a hardware-recognized secondary virtual display in the Windows OS display topology, paired with zero-copy DirectX 11 GPU capture, hardware-accelerated video encoding, and a multi-threaded network streaming engine.

### Complete Windows Media Pipeline:
```
SecondScreen IddCx Virtual Display (Display 2)
              │
              ▼
   DirectX 11 / DXGI Desktop Duplication (DXGICaptureManager)
              │
              ▼
  ID3D11Texture2D (Zero-Copy VRAM)
              │
              ▼
 Hardware Video Encoder (NVENC / QuickSync / AMF)
              │
              ▼
 Binary Packet Framer (Protocol v1 / PacketHeader)
              │
              ▼
 Asynchronous WinSock2 Server (TCP 9876 Control + Port 9878 Video)
              │
              ▼
 Local 5GHz Wi-Fi / LAN
              │
              ▼
 Android Client (MediaCodec -> SurfaceView)
```

---

## 2. Prerequisites & Environment Setup

### Required Tools on Windows Host:
1. **Operating System**: Windows 10 64-bit (Version 2004 / Build 19041+) or Windows 11.
2. **Visual Studio 2022**: Community, Professional, or Enterprise.
   - Workload: *Desktop development with C++*.
   - Optional component: *MSVC v143 - VS 2022 C++ x64/x86 build tools*.
3. **Windows Driver Kit (WDK)**:
   - Install WDK for Windows 10/11 matched to the Windows SDK build number.
   - Verify Visual Studio WDK extension is active.
4. **GPU Drivers & SDKs**:
   - NVIDIA: GeForce Game Ready / Studio Driver 522+ with `nvEncodeAPI64.dll`.
   - Intel: Intel Graphics Driver with oneVPL / Media SDK runtime (`libmfxhw64.dll` / `vpl.dll`).
   - AMD: AMD Software Adrenalin Edition with `amfrt64.dll`.

---

## 3. Directory Layout
```
native/windows/
├── driver/
│   ├── Driver.h                  # IddCx structures and EDID block (1080p -> 4K)
│   ├── Driver.cpp                # WDF DriverEntry & DeviceAdd
│   ├── Device.cpp                # Monitor arrival, target modes, swapchain worker
│   ├── SecondScreenIddCx.inf     # Driver installation package manifest
│   └── SecondScreenIddCx.vcxproj # Visual Studio + WDK project
├── capture/
│   ├── DXGICapture.h             # Zero-copy GPU frame grabber header
│   └── DXGICapture.cpp           # Direct3D 11 / DXGI Desktop Duplication pipeline
├── encoder/
│   ├── HardwareEncoder.h         # Unified encoder interface & Factory
│   ├── HardwareEncoder.cpp       # GPU capability detection
│   ├── NVENCEncoder.h/.cpp       # NVIDIA NVENC low-latency CBR encoder
│   ├── QuickSyncEncoder.h/.cpp   # Intel QuickSync hardware encoder
│   └── AMFEncoder.h/.cpp         # AMD AMF hardware encoder
├── network/
│   ├── NetworkServer.h           # WinSock2 control & streaming server header
│   └── NetworkServer.cpp         # Multi-threaded server implementation
└── scripts/
    ├── build_driver.bat          # Command-line MSBuild script
    └── install_driver.ps1        # Elevated PowerShell driver installer
```

---

## 4. Step-by-Step Compilation & Installation

### Step 1: Compile the IddCx Driver
1. Open Developer Command Prompt for Visual Studio 2022 as Administrator.
2. Execute:
   ```cmd
   cd native\windows\scripts
   build_driver.bat
   ```

### Step 2: Enable Test-Signing & Install Driver
1. In an elevated PowerShell prompt:
   ```powershell
   bcdedit /set testsigning on
   ```
   *(Reboot system if test-signing was newly enabled)*.
2. Run the installer:
   ```powershell
   cd native\windows\scripts
   .\install_driver.ps1
   ```

### Step 3: Verify OS Virtual Display
1. Open **Windows Settings → System → Display**.
2. Confirm **Display 2** is listed as `SecondScreen Virtual Display`.
3. Select **Extend these displays** with resolution `1920x1080` (or `2560x1440`).

---

## 5. GPU Capture, Encoding & Network Testing

### Run Host Streaming Pipeline:
1. Start the host streaming service with target display selection:
   - `DXGICaptureManager` automatically enumerates active displays and attaches to the SecondScreen Virtual Display.
   - `HardwareEncoderFactory` detects the active GPU (NVENC, QuickSync, or AMF).
   - `NetworkServer` opens port 9876 (Control) and starts the discovery beacon on UDP 9877.
2. Observe telemetry logs:
   - Latency breakdown is measured per frame using microsecond precision.
   - Frame acquisition to network packetization pipeline achieves sub-5ms host processing latency.

---

## 6. Troubleshooting
- **`DXGI_ERROR_ACCESS_LOST`**: Occurs when display mode changes or UAC secure desktop appears. Handled automatically by `DXGICaptureManager::Reinitialize()`.
- **`nvEncodeAPI64.dll not found`**: Host does not have an NVIDIA GPU or driver installed. Factory falls back gracefully to Intel QuickSync or AMD AMF.
- **Firewall Prompt**: Allow inbound connections on TCP 9876 and UDP 9877/9878 for LAN streaming.
