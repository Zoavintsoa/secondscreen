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

LegacyCapture isolates the old-Mac capture path. It remains a separate compatibility boundary. ScreenCaptureKit is an optional modern capture backend and must not be required for Catalina/Big Sur compatibility.

### Encoding

VideoToolboxEncoder is the common encoder boundary. H.264 is the mandatory baseline. HEVC is optional and must be negotiated from runtime capability.

The current VideoToolbox implementation is a capability/architecture scaffold only; it does not yet represent a production end-to-end encoder.

### Virtual display

The public macOS APIs currently used by this project do not provide a supported way for an ordinary application to create an OS-level virtual monitor. The branch `feat/macos-experimental-virtual-display` contains an isolated **experimental** prototype using undocumented CoreGraphics runtime classes.

That prototype is for local engineering validation only. It may fail or break across macOS releases, is not a production compatibility guarantee, and is not ready for Mac App Store distribution. A button response is not sufficient validation: the monitor must appear in System Settings → Displays and accept a normal application window.

Removing the private display object is best-effort; some macOS builds may keep the monitor until the process exits. If it remains after selecting Remove monitor, quit SecondScreen and verify cleanup.

The existing test stream still captures the primary display. The prototype does **not** yet route frames from the virtual display to Android/iPadOS.

See [the virtual display prototype validation guide](../../docs/macos-virtual-display-prototype.md).

## Product target

A validated old-Mac release must provide:

1. Real virtual display creation.
2. LAN discovery and authenticated pairing.
3. H.264 low-latency streaming from the selected display.
4. Android and iPadOS clients.
5. Adaptive resolution/FPS/bitrate.
6. 30 FPS compatibility mode on constrained Intel Macs.
7. 60 FPS only when runtime capability and measured performance allow it.

No item is considered complete from a code stub or simulated result alone.
