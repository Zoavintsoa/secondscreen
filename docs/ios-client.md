# SecondScreen — iOS & iPadOS Client Architecture

## 1. Overview
The iOS / iPadOS Client subsystem turns Apple iPads and iPhones into high-resolution, low-latency remote displays with full multi-touch and Apple Pencil support.

---

## 2. Architecture & Pipeline
1. **Network Layer (`SecondScreenIOSClient.swift`)**:
   - Built on Apple's modern `Network.framework` (`NWConnection`).
   - Handles discovery, 6-digit PIN pairing, and binary packet reception.
2. **Hardware Decompression (`VTDecoder.swift`)**:
   - `VTDecompressionSession` processes H.264 NALUs with zero-copy Direct3D/Metal compatibility flags.
3. **Metal Rendering Viewport (`MetalVideoView.swift`)**:
   - `MTKView` using `CVMetalTextureCache` renders decoded `CVPixelBuffer` textures at up to 120Hz (ProMotion on iPad Pro).
4. **Apple Pencil & Multi-Touch Forwarding**:
   - Normalizes coordinates `(0.0 - 1.0)` with sub-pixel precision.
   - Forwards force (pressure) and altitude (tilt) back to the host operating system.
