# SecondScreen roadmap

## Phase 1 — Windows + Android vertical slice

### Foundation

- [x] Native repository structure
- [x] Versioned protocol
- [x] LAN discovery foundation
- [x] Pairing manager foundation
- [x] Windows host executable foundation
- [x] Android MediaCodec rendering foundation
- [x] IddCx driver source foundation based on Microsoft's official sample
- [x] Experimental GPU shared-resource frame bridge
- [x] GPU NV12 conversion path
- [x] Media Foundation H.264 encoder path
- [x] TCP video bring-up path
- [x] Separate discovery/control/video port model
- [x] Product vision and Smart Stream architecture

### Production path — not hardware accepted yet

- [ ] Harden IddCx frame handoff and keep driver frame loop minimal
- [x] Harden pairing credential comparison against timing leaks
- [x] MsQuic host transport implementation (Windows listener, TLS credentials, control stream, negotiated datagram size, SSVG send path; hardware acceptance pending)
- [ ] MsQuic Android transport adapter (JNI boundary and safe unavailable stub added)
- [ ] Authenticated pairing end-to-end
- [x] Bounded control-frame parser and framing implementation
- [x] Transport-neutral control session state machine with host security callbacks
- [x] QUIC datagram video fragmentation contract
- [x] Android STREAM_CONFIG parser/reconfiguration path (host policy still fixed until Smart Engine integration)
- [x] Android keyframe-request/recovery hooks (QUIC loss callback integration still pending)
- [ ] Adaptive bitrate/FPS controller
- [ ] Real virtual display -> Android video path
- [ ] Touch/stylus return channel
- [ ] End-to-end diagnostics
- [ ] Real Windows-to-Android hardware acceptance

## Phase 2 — Windows productization

- [ ] Installer
- [ ] Driver test-signing installer flow
- [ ] Firewall rule management
- [ ] Tray application
- [ ] Multi-monitor management
- [ ] 1440p/4K profiles
- [ ] 60/120 Hz where hardware allows
- [ ] Diagnostics/latency overlay
- [ ] Crash recovery and service watchdog
- [ ] Trusted-device management
- [ ] Workspace profiles

## Phase 3 — Creator platform

- [ ] Creator Preview
- [ ] Creator Scopes
- [ ] Camera Monitor
- [ ] Tablet/stylus mode
- [ ] DaVinci/Premiere/OBS/vMix extension interfaces
- [ ] Local extension SDK

## Phase 4 — macOS + iPadOS

- [ ] Intel macOS host
- [ ] Apple Silicon host
- [ ] Virtual display implementation based only on supported Apple APIs
- [ ] ScreenCaptureKit capture
- [ ] VideoToolbox encode
- [x] iPadOS 13 compatibility floor and capability detection foundation
- [ ] Metal renderer
- [ ] Apple Pencil/touch input

## Phase 5 — Advanced

- [ ] Multiple simultaneous clients
- [ ] Multiple virtual monitors
- [ ] Audio transport
- [ ] USB/HID extensions
- [ ] HDR/color-management profiles
- [ ] Network-quality adaptive profiles
- [ ] Camera/microphone roles

### Rule

A checkbox is not marked complete because code exists. It is complete only after the corresponding real-hardware acceptance test is passed.


## 2026-10-08 implementation pass
- Windows control channel now uses the shared SSCP parser/session and PairingManager callbacks.
- Pairing uses a short-lived six-digit host challenge and a separate 256-bit session token.
- Android persists the session token locally and prompts for the host pairing code only when needed.
- Video bring-up is now gated on an authenticated control session.
- Native macOS and iPadOS Xcode projects were added and are included in CI.
- MsQuic production transport remains the next transport implementation; no hardware acceptance is claimed yet.

### 2026-10-08 compatibility pass
- Android target is capability-driven from API 26 upward: codec, resolution, frame-rate, HDR, refresh-rate, touch and stylus probing are implemented.
- Android H.264 remains the baseline; HEVC is selected only when negotiated.
- A native QUIC JNI boundary is present and deliberately reports unavailable until upstream MsQuic is actually linked and validated; TCP is never mislabeled as QUIC.
- iPadOS deployment target is 13.0 with availability-safe capability probing for H.264, HEVC, Metal and Pencil.
- No device-specific manufacturer/model assumptions were introduced.
