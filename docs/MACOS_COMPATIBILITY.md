# macOS Compatibility

SecondScreen treats older Intel Macs as a first-class host target.

Apple lists MacBook Pro late 2013 as compatible with macOS Big Sur, while early 2013 MacBook Pro models top out at macOS Catalina. The host therefore cannot make ScreenCaptureKit or Sidecar a hard dependency.

## Supported legacy target

- macOS 10.15 Catalina: legacy path
- macOS 11 Big Sur: legacy + modern capability paths
- newer Intel macOS: modern path with legacy fallback where useful
- Apple Silicon: modern path

The exact oldest supported hardware is capability-driven rather than model-name-driven.

## Host pipeline

Display capability detection -> capture backend -> VideoToolbox H.264 -> transport -> Android/iPadOS

### Capture

The legacy implementation must use APIs available on Catalina/Big Sur. ScreenCaptureKit is optional and must never be required for the legacy target.

The first implementation target is a CoreGraphics-compatible capture backend. The code must isolate capture behind an interface so a newer ScreenCaptureKit backend can coexist without changing transport or encoding.

### Encoding

VideoToolbox is the common encoding boundary. The host uses VTCompressionSession and probes available encoders at runtime.

H.264 is the baseline codec for old Macs. HEVC is optional and negotiated only when the runtime reports support.

The encoder must expose actual capability results; it must never label software or hardware encoding as available without probing.

### Transport

The host transport remains independent of capture and encoding. QUIC is preferred when the runtime transport is available. A deliberately marked bring-up fallback can remain for development, but it is not the production architecture.

### Virtual display

A true virtual display is a separate component from screen capture. SecondScreen will not use undocumented/private display APIs.

The project must not claim "virtual monitor" support on legacy macOS until a public, supportable display-creation mechanism is validated. Capture-only operation is useful for development but does not satisfy the product definition.

## Product guarantees

- No Sidecar dependency.
- No model-specific assumptions.
- Runtime capability detection.
- H.264 baseline.
- Adaptive resolution/FPS/bitrate.
- 30 FPS compatibility mode for constrained Intel hardware.
- 60 FPS only when measured capability allows it.
- No private Apple display APIs.

## Validation rule

Documentation and code scaffolding may establish compatibility intent. A legacy Mac is not marked hardware-accepted until the real device is tested end-to-end.
