# SecondScreen — System Architecture Specification

## 1. Core Vision & Product Definition
SecondScreen is a high-performance, general-purpose cross-platform virtual secondary display system. It transforms portable Android tablets, iPads, and iPhones into genuine, low-latency secondary monitors for Windows and macOS workstations.

Unlike application-specific viewers or browser canvas tools, SecondScreen creates a genuine virtual display in the host operating system (`Display 2`). Any desktop application supporting multiple monitors (DaVinci Resolve, Cubase, FL Studio, Adobe Creative Cloud, OBS Studio, Blender, Unreal Engine, Web Browsers, CAD tools, etc.) can be moved or extended onto Display 2 without plugins or modifications.

---

## 2. Four-Way Cross-Platform Topology

```
                              SECOND SCREEN
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
             WINDOWS HOST                      macOS HOST
                    │                               │
       UMDF 2.0 IddCx Driver               Virtual Display
                    │                               │
            DirectX 11 / DXGI                ScreenCaptureKit
                    │                               │
             Hardware Encoder                 VideoToolbox
          (NVENC / QSV / AMF)                (Apple Silicon)
                    │                               │
                    └───────────────┬───────────────┘
                                    │
                         SECOND SCREEN PROTOCOL
                           (TCP 9876 / Port 9878)
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
             ANDROID CLIENT                  iOS / iPadOS CLIENT
                    │                               │
           MediaCodec (Low-Lat)                VideoToolbox
                    │                               │
          SurfaceView (Zero-Copy)             Metal (MTKView)
                    │                               │
                    └───────────────┬───────────────┘
                                    │
                           TRUE FULLSCREEN MODE
                               (Display 2)
```

---

## 3. Layered Architecture Separation

1. **`core/` (Platform-Independent Protocol Engine)**:
   - Wire Protocol v1 packet framing (`PacketHeader`, magic `0x53325343 'S2SC'`).
   - Discovery, authentication, 6-digit dynamic PIN handshake, session token management.
   - Real microsecond timestamp synchronization (Capture -> Encode -> Network -> Decode -> Render).
   - Normalized multi-touch and stylus digitizer coordinate mapping `(0.0 - 1.0)`.

2. **`native/windows/` (Windows Host Subsystem)**:
   - User-Mode Driver Framework (UMDF 2.0) `IddCx` Indirect Display Driver exposing hardware-recognized Display 2.
   - DirectX 11 / DXGI Desktop Duplication zero-copy GPU texture frame acquisition (`DXGICaptureManager`).
   - Hardware Encoder Factory supporting NVIDIA NVENC, Intel QuickSync, and AMD AMF.
   - WinSock2 asynchronous multi-threaded streaming server.

3. **`native/macos/` (macOS Host Subsystem)**:
   - macOS Virtual Display Manager (`CGVirtualDisplay` CoreDisplay engine).
   - ScreenCaptureKit (`SCStream`) zero-copy GPU texture capture mapped to Metal.
   - Apple VideoToolbox hardware H.264/HEVC encoding utilizing M-series Media Engines.
   - Network.framework TCP/UDP streaming server.

4. **`native/android/` (Android Client Subsystem)**:
   - Kotlin / Android SDK 34 (minSdk 26).
   - Android `MediaCodec` low-latency decoder writing directly to `SurfaceView`.
   - Touch and S-Pen pressure/tilt digitizer serialization.
   - Immersive sticky fullscreen activity.

5. **`native/ios/` (iOS / iPadOS Client Subsystem)**:
   - Swift / iOS 15+ / iPadOS 15+.
   - VideoToolbox `VTDecompressionSession` hardware decompression.
   - MetalKit `CAMetalLayer` / `MTKView` GPU rendering.
   - Apple Pencil pressure and tilt serialization.
   - True fullscreen viewport.
