# SecondScreen — Windows Host Architecture & Build Guide

## 1. Overview
The Windows Host component creates a genuine secondary monitor in the Windows OS display topology using the User-Mode Driver Framework (UMDF 2.0) and Indirect Display Driver model (`IddCx`).

---

## 2. Pipeline Stages
1. **Virtual Display Creation**:
   - Driver package `SecondScreenIddCx.dll` + `SecondScreenIddCx.inf`.
   - Windows Desktop Window Manager (DWM) recognizes Display 2.
   - User can Extend, Duplicate, or configure resolutions up to 4K @ 60Hz.
2. **Zero-Copy GPU Capture**:
   - `DXGICaptureManager` connects via `IDXGIOutputDuplication` to Display 2.
   - Yields `ID3D11Texture2D` in VRAM without CPU memory staging.
3. **Hardware Video Encoding**:
   - `HardwareEncoderFactory` detects NVIDIA NVENC, Intel QuickSync, or AMD AMF.
   - Encodes H.264 video at 20–25 Mbps CBR with sub-4ms latency.
4. **WinSock2 Server**:
   - TCP 9876 handles 6-digit PIN pairing, session management, and binary framed packet streaming.

---

## 3. Compilation & Installation
1. Open Developer Command Prompt for Visual Studio 2022 as Administrator.
2. Execute `build_driver.bat` in `native/windows/scripts/`.
3. Run `install_driver.ps1` in an elevated PowerShell session to install the test certificate and driver package into the Windows Driver Store.
