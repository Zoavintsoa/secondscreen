# Engineering Review — SecondScreen

## Current conclusion

The Windows architecture is technically sound for a true virtual display because Microsoft explicitly positions IddCx for remote-display streaming and virtual-monitor scenarios. The official IddCx sample provides the adapter, monitor and swap-chain lifecycle we need; the production work is replacing its frame-discard section with a real frame pipeline.

The Windows path is the primary implementation track:

OS virtual monitor
-> IddCx swap-chain
-> GPU frame bridge
-> hardware H.264 encoder
-> QUIC datagram
-> Android MediaCodec Surface
-> fullscreen presentation

## Hard architectural rule

The display driver must not become the application server.

Driver responsibilities:
- virtual monitor lifecycle;
- supported modes;
- swap-chain ownership;
- frame acquisition;
- minimal frame handoff;
- driver telemetry.

Host-service responsibilities:
- pairing;
- discovery;
- QUIC;
- encoder;
- bitrate adaptation;
- session management;
- diagnostics;
- user settings;
- input routing.

This isolates network failures from the Windows display path.

## Frame pipeline

Preferred path:

1. IddCx supplies a GPU-backed DXGI surface.
2. A frame bridge transfers ownership/reference to an asynchronous processing queue.
3. A GPU-compatible color conversion produces encoder input when required.
4. Media Foundation H.264 hardware MFT encodes the frame.
5. The access unit is normalized to Annex-B.
6. The host puts the access unit on the QUIC datagram path.
7. Android decodes directly to a Surface.
8. Presentation timestamps are used for latency accounting.

CPU readback is a fallback/debug path, not the final performance path.

## Encoder strategy

H.264 is the first mandatory codec because it has broad hardware decoder support and is directly supported by Media Foundation. The encoder must:
- prefer a hardware MFT;
- use low-latency mode;
- accept NV12;
- produce H.264;
- support forced keyframes;
- expose encoder latency and queue depth;
- fall back to software only when hardware encoding is unavailable.

The first hardware profile is 1920x1080 at 60 FPS. 720p30 is the safe fallback.

## Adaptive quality controller

Do not adapt from FPS alone.

Inputs:
- encode time;
- decode time;
- QUIC RTT;
- packet loss;
- jitter;
- queue depth;
- thermal throttling where available;
- rendered FPS.

Controller:
- increase bitrate slowly when the path is healthy;
- decrease bitrate quickly after sustained congestion;
- reduce FPS only after bitrate reduction;
- reduce resolution when target latency remains unattainable;
- force a keyframe after a profile change.

Use hysteresis and minimum dwell times to avoid oscillation.

## Android renderer

Android must not assume 1920x1080 forever.

The client should:
- receive STREAM_CONFIG;
- create the decoder using negotiated dimensions;
- recreate only when codec/profile changes;
- use Surface output;
- drop stale frames rather than blocking the render path;
- request a keyframe after decoder reset or loss recovery.

## macOS feasibility

Apple's public APIs clearly support high-performance screen capture through ScreenCaptureKit and QUIC through Network.framework. ScreenCaptureKit is intended for screen streaming/mirroring and delivers CMSampleBuffer data.

However, current public DriverKit documentation describes USB, HID, networking, audio and related device families; it does not expose a general-purpose third-party virtual-display driver family equivalent to Windows IddCx.

Therefore SecondScreen must not pretend that ScreenCaptureKit creates a virtual monitor.

macOS track:
1. native host app;
2. supported Apple screen-capture APIs;
3. VideoToolbox encoding;
4. Network.framework QUIC;
5. investigate only supported/public mechanisms for a true virtual display;
6. if a supported virtual-display mechanism is unavailable, keep macOS host in capture/stream mode until a legitimate display-driver path exists.

No private API, undocumented display hack or fragile kernel extension.

## iPadOS

iPadOS client:
- native Swift/SwiftUI shell;
- Network.framework QUIC;
- hardware H.264/HEVC decode;
- Metal presentation;
- touch and Apple Pencil input;
- persistent pairing;
- reconnect/keyframe recovery.

The same protocol is used across Windows, macOS, Android and iPadOS.

## Product differentiation

The product should not compete only on "it shows a second screen".

Differentiators:
- native virtual monitor;
- low-latency LAN operation;
- no cloud account;
- hardware acceleration;
- automatic quality control;
- one-tap reconnect;
- persistent trusted-device pairing;
- touch/stylus input;
- transparent latency diagnostics;
- cross-platform protocol;
- creator-oriented presets for DaVinci Resolve, Premiere Pro and live-production workflows.

## Quality gates

A feature is accepted only after real-hardware validation.

1. Windows sees a real SecondScreen monitor.
2. Host receives real IddCx frames.
3. Real H.264 access units are produced.
4. Android displays the real stream.
5. Reconnect recovers without application restart.
6. Authenticated input reaches the host.
7. Adaptive quality maintains usable latency.
8. Installer/uninstaller and driver recovery are reliable.

No simulator checkbox can satisfy these gates.

## Current state

Implemented foundation:
- native repository structure;
- protocol v1.1;
- Windows IddCx driver source foundation based on Microsoft's official sample;
- Windows host skeleton;
- secure pairing foundation;
- UDP discovery bring-up;
- Android MediaCodec renderer foundation;
- CI build definitions;
- macOS/iPadOS architecture.

Still required for the first real product:
- connect IddCx frames to the host frame pipeline;
- real Media Foundation H.264 encoding;
- production QUIC;
- authenticated end-to-end handshake;
- dynamic Android stream configuration;
- keyframe recovery;
- real Windows-to-Android hardware acceptance.
