# SecondScreen — Transport Test Plan

## Phase A — build validation

Required:
- Android build succeeds.
- macOS build succeeds.
- Windows build succeeds.
- iPadOS build succeeds.

A GitHub Actions success is a build validation, not hardware validation.

## Phase B — macOS → Android USB

Hardware:
- macOS host
- Android 14 device
- data-capable USB cable

Procedure:
1. Install the latest Android APK.
2. Install the latest macOS host.
3. Start SecondScreen on Android.
4. Select USB or AUTO.
5. Connect the USB cable.
6. Accept the Android accessory permission if shown.
7. Start/restart the macOS host.
8. Verify USB connected / direct video status.
9. Move windows or play video on the Mac.
10. Measure visible latency.
11. Disconnect cable for 5 seconds.
12. Reconnect.
13. Verify automatic recovery and keyframe lock.

Expected:
- No LAN is required in USB mode.
- Video appears without the Wi-Fi path.
- Reconnect recovers on a keyframe.
- USB mode does not silently fall back to Wi-Fi.

## Phase C — macOS → Android AUTO

1. Enable Wi-Fi.
2. Connect USB.
3. AUTO must prefer USB.
4. Remove USB.
5. AUTO must return to LAN discovery.
6. Reconnect USB.
7. AUTO must prefer USB again.

## Phase D — Windows → Android USB

1. Build/install Windows host.
2. Connect Android with a data cable.
3. Verify AOA device detection.
4. Verify accessory-mode re-enumeration.
5. Verify SSVF transfer.
6. Verify decoder/keyframe recovery.
7. Disconnect/reconnect.

## Phase E — iPadOS

Use LAN/TCP or QUIC first.

USB is not considered a supported direct PC-to-iPad application transport until an Apple-supported accessory architecture is available.

## Measurements

Record:
- connection time
- first-frame time
- average displayed latency
- reconnect time
- dropped frames
- keyframe recovery time
- CPU/GPU usage
- encoded bitrate
- USB/LAN mode

A test is PASS only when observed on physical hardware.
