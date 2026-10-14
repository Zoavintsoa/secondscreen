# macOS Host

SecondScreen supports a dedicated legacy Intel path so older Macs are not blocked by modern-only APIs.

## Compatibility

- macOS 10.15 Catalina: legacy compatibility target
- macOS 11 Big Sur: legacy path plus modern capability path
- newer Intel macOS: modern path with legacy fallback
- Apple Silicon: modern path

Apple identifies MacBook Pro late 2013 as Big Sur-compatible; early 2013 models top out at Catalina. Sidecar is not the dependency for SecondScreen.

## Architecture

Display capability detection -> capture backend -> VideoToolbox -> transport -> Android/iPadOS.

### Legacy capture

LegacyCapture isolates the old-Mac capture path. The implementation is intentionally based on public CoreGraphics-era APIs and does not use private display APIs.

ScreenCaptureKit remains an optional modern backend. It must never be required for Catalina/Big Sur compatibility.

### Encoding

VideoToolboxEncoder is the common encoder boundary. H.264 is the mandatory baseline. HEVC is optional and must be negotiated from runtime capability.

The current VideoToolbox implementation is a capability/architecture scaffold only; it deliberately does not pretend that a production encoder is complete.

### Virtual display

Capture and virtual-display creation are separate problems. A previous experimental virtual-display implementation has been restored behind an explicit warning. It uses undocumented private CoreGraphics runtime classes, is not a supported Apple API, and may stop working across macOS releases. It is not suitable to present as a production-ready or distribution-safe feature. Sidecar is not used.

The test stream still captures the main display; virtual-display frame capture is not yet connected. A supported production implementation remains a separate requirement.

## Product target

A validated old-Mac release must provide:

1. Real virtual display creation.
2. LAN discovery and authenticated pairing.
3. H.264 low-latency streaming.
4. Android and iPadOS clients.
5. Adaptive resolution/FPS/bitrate.
6. 30 FPS compatibility mode on constrained Intel Macs.
7. 60 FPS only when the runtime and measured performance allow it.
