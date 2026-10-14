# SecondScreen — macOS host: verified status and recovery plan

## Verified status in this repository

The current macOS host is an **experimental screen-mirroring test stream**, not a complete virtual-display product.

- The test stream captures `CGMainDisplayID()` at 1280×720 and targets 30 FPS.
- It encodes H.264 with a `VTCompressionSession` and sends framed video over TCP port 49153.
- Android decodes H.264 with `MediaCodec`.
- Discovery is broadcast over UDP port 49151.
- The app's virtual-display action deliberately reports that creation is unavailable.
- `native/macos/display/MacVirtualDisplay.m` is a stub. It does not create a system display.
- `native/macos-host/VideoToolboxEncoder.mm` is a capability/architecture scaffold; it is not the active test-stream encoder.

Consequently, a display shown in macOS Settings is not proof that this repository's current virtual-display manager created it. Check which installed build or other software owns that display before relying on it.

## Stream recovery

A newly connected Android decoder waits for an H.264 keyframe before rendering. If the host only sends the next periodic keyframe, the client may show black after a disconnect. The test-stream encoder now requests a keyframe when a client connects and exposes whether startup reached its running state. Android codec configuration failures are surfaced instead of being silently swallowed.

These changes improve recovery and diagnostics; they do not prove that every network or codec failure is fixed. Real-device validation remains required.

## Real virtual display versus mirroring

A true extended display requires all of the following:

1. A supported macOS display-creation mechanism for the target macOS version and signing/distribution model.
2. Capturing the **created virtual display's display ID**, not unconditionally capturing the main iMac display.
3. A stable H.264 pipeline and transport with keyframe recovery, bounded queues, and reconnection handling.
4. Explicit display controls: create/stop, resolution/refresh rate, extend/mirror mode where supported, and a visible way to restore the built-in display as primary.
5. End-to-end tests on the target iMac and Android device.

The repository does not currently implement this complete path. Do not describe the macOS host as a working extended display until these requirements pass on hardware.

## Safe recovery when windows move to another display

Use macOS **System Settings → Displays** to select the display and choose mirroring or rearrange displays. These system controls are separate from the Android video stream. The host UI should not claim that the Android image has arrived merely because the capture start request returned.

## Validation checklist

- [ ] Start succeeds only when capture and encoder initialization succeed.
- [ ] A fresh client receives a keyframe promptly.
- [ ] Disconnect/reconnect repeatedly without remaining on a black frame.
- [ ] Screen changes continue updating after 10 minutes.
- [ ] Permission denial and capture failure are visible in the host UI.
- [ ] macOS build and Android build pass in CI.
- [ ] A real virtual display is created and its frames, not the main display's frames, appear on Android.
- [ ] A safe duplicate/extend/restore workflow is validated on the target macOS version.
