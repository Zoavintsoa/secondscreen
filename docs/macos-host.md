# SecondScreen — macOS Host Architecture & Technical Truth

## 1. Overview
The macOS Host subsystem provides genuine OS Display 2 creation, ScreenCaptureKit GPU capture, and VideoToolbox hardware acceleration for macOS 12.3 Monterey, macOS 13 Ventura, macOS 14 Sonoma, and macOS 15 Sequoia.

---

## 2. Technical Truth & API Investigation

### Virtual Display Allocation on macOS:
1. **CoreDisplay Private Framework (`CGVirtualDisplay`)**:
   - `CGVirtualDisplayDescriptor` and `CGVirtualDisplay` allow dynamic allocation of recognized displays without system extensions or kernel panics.
   - Requires TCC Screen Recording permissions (`System Settings → Privacy & Security → Screen Recording`).
   - Used by production remote desktop applications on Apple Silicon and Intel Macs.
2. **DriverKit System Extensions (`IOUserGraphicsDriver` / `IOUserDisplayDriver`)**:
   - Production-grade public Apple interface.
   - **Limitation**: Requires a special restricted entitlement (`com.apple.developer.driverkit.user-client-access`) granted explicitly by Apple Developer Relations.

---

## 3. Media Pipeline
```
macOS Display Manager (CGVirtualDisplay)
          │
          ▼
ScreenCaptureKit (SCStream @ 60 FPS)
          │
          ▼
CVImageBuffer / CVPixelBuffer (Zero-Copy Metal)
          │
          ▼
Apple VideoToolbox (VTCompressionSession)
          │
          ▼
Network.framework (TCP 9876 + UDP 9878)
          │
          ▼
Client (Android / iPadOS)
```

---

## 4. Requirements & Setup
- **OS**: macOS 12.3 or higher (Apple Silicon M1/M2/M3/M4 or Intel Core).
- **IDE**: Xcode 15 / 16 with macOS SDK 14+.
- **Permissions**: TCC Screen Recording entitlement granted in macOS System Settings.
