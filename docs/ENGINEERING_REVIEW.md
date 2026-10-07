# Engineering Review — SecondScreen

## Current conclusion

The Windows architecture is technically appropriate for a real virtual-display product. Microsoft's IddCx model provides the virtual adapter, monitor and swap-chain lifecycle, and the OS supplies the desktop image as a DirectX surface.

The production path remains:

OS virtual monitor
-> IddCx swap-chain
-> minimal GPU frame handoff
-> hardware H.264/HEVC encoder
-> QUIC datagram
-> Android MediaCodec Surface
-> fullscreen presentation

The repository now contains experimental implementations of several links in this chain. They are intentionally still below the real-hardware acceptance gate.

## Hard architectural rule

The display driver must not become the application server.

Driver responsibilities:
- virtual monitor lifecycle;
- supported modes;
- swap-chain ownership;
- frame acquisition;
- minimal GPU frame publication;
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

Microsoft's IddSample guidance also recommends keeping significant work out of the frame-processing loop because it directly affects system performance.

## Current frame bridge

The current Windows branch contains an experimental triple-buffered shared D3D11 resource bridge between the IddCx driver process and the host service.

Design:
- BGRA shared textures;
- keyed mutex synchronization;
- shared state mapping;
- ready event;
- adapter LUID matching;
- sequence counter to reject torn metadata snapshots.

This is an engineering bridge, not yet a final production IPC contract.

Before acceptance it must be hardened for:
- lifecycle/reconnect;
- ACL minimization;
- device/driver disappearance;
- mode changes;
- resource recreation;
- non-BGRA source formats;
- frame-loop timing.

## Encoder

The host contains a Media Foundation H.264 path:
- hardware MFT preferred;
- software fallback;
- low-latency property when supported;
- NV12 input;
- GPU-side conversion;
- Annex-B normalization;
- sequence-header handling;
- explicit keyframe request.

The encoder is still below acceptance because MFT behavior varies by hardware/driver and must be validated on the actual target PCs.

Keyframe state is deliberately owned by the encoder/session. A display-frame sequence number is never treated as a keyframe indicator.

## Transport

The current TCP video server is a deterministic bring-up path only.

Port separation:
- UDP discovery: 49151;
- future control: 49152;
- temporary video TCP: 49153.

Production uses MsQuic:
- reliable stream for control;
- datagrams for video.

MsQuic support covers Windows and Linux/Android, making it suitable for a common application protocol with native platform adapters.

## Android

The Android client currently has:
- discovery;
- reconnect loop;
- TCP bring-up receiver;
- MediaCodec Surface rendering;
- presentation timestamps.

Still required:
- QUIC adapter;
- authenticated HELLO/AUTH;
- dynamic STREAM_CONFIG;
- keyframe recovery;
- stale-delta rejection;
- input channel;
- telemetry.

## Smart Stream Engine

The product architecture now defines a local controller using:
- RTT;
- jitter;
- loss;
- encode/decode time;
- queue depth;
- presented FPS;
- thermal/battery hints;
- scene activity.

The controller must degrade quickly and recover slowly, with hysteresis and minimum dwell times.

## Product differentiation

SecondScreen is designed as a virtual-display core plus workspace roles:
- Creator Preview;
- Creator Scopes;
- Camera Monitor;
- Tablet Input;
- Gaming Low Latency.

This creates a product surface beyond generic screen mirroring without destabilizing the display driver.

## macOS

ScreenCaptureKit and VideoToolbox are appropriate for capture/encoding workflows, but they must not be described as creating a third-party virtual monitor.

The macOS track must use only supported/public APIs. If a legitimate virtual-display mechanism is unavailable, the product should retain a capture/stream mode rather than using private APIs.

## Quality gates

A feature is accepted only after real-hardware validation:

1. Windows sees a real SecondScreen monitor.
2. Host receives real IddCx frames.
3. Real H.264 access units are produced.
4. Android displays the real stream.
5. Reconnect recovers without application restart.
6. Authenticated input reaches the host.
7. Adaptive quality maintains usable latency.
8. Installer/uninstaller and driver recovery are reliable.

No simulator checkbox satisfies these gates.

## Next implementation order

1. MsQuic transport abstraction.
2. End-to-end control/auth protocol.
3. Android dynamic stream configuration and keyframe recovery.
4. Harden the GPU frame bridge.
5. Smart Stream Engine implementation.
6. Input channel.
7. Diagnostics.
8. Workspace profiles.
9. Windows productization.
10. macOS/iPadOS native tracks.
