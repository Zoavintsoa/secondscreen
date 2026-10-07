# SecondScreen

SecondScreen is a native, cross-platform virtual-display and local workspace platform.

A computer creates a real secondary display and streams it over the local network to an Android device, iPad, or another supported client.

## Product idea

**Your devices become your workspace.**

The core remains a real operating-system virtual display. Above that core, SecondScreen adds adaptive streaming, trusted pairing, touch/stylus input and workflow roles for creators, camera monitoring and low-latency use.

This repository is not a web page. The old web prototype is not the product.

## Native architecture

### Windows host
- Windows Indirect Display Driver (IddCx)
- GPU-backed frame bridge
- Media Foundation hardware H.264/HEVC encoding
- MsQuic production transport
- TCP bring-up path only
- pairing/authentication
- Smart Stream Engine
- display mode and profile management

### macOS host
- supported/public virtual-display path under investigation
- ScreenCaptureKit for supported capture/streaming workflows
- VideoToolbox hardware encoding
- native QUIC transport
- no undocumented/private display API

### Android client
- Kotlin
- MediaCodec hardware decoder
- Surface rendering
- fullscreen and rotation
- touch/stylus input channel
- automatic discovery and pairing
- diagnostics

### iPadOS client
- Swift
- VideoToolbox
- Metal rendering
- touch/Pencil input

## Product roles

Initial architecture supports:
- normal second display;
- Creator Preview;
- Creator Scopes;
- Camera Monitor;
- Tablet/pen input;
- Gaming Low Latency.

These roles are layered above the core display and transport path.

## Product identity

**SecondScreen — Zoavintsoa**

The product identity is intentionally embedded in the native codebase, application identifiers, desktop/mobile metadata and user-facing About surfaces. The original project identity must remain distinguishable from third-party dependencies.

Legal and product documents:
- TERMS_OF_USE.md
- PRIVACY.md
- ACCESSIBILITY.md
- ABOUT.md
- THIRD_PARTY_NOTICES.md

## Engineering rules

1. Native functionality takes priority over UI mockups.
2. No feature is considered complete until it works on real hardware.
3. Simulation/prototype code must never be presented as a real virtual display.
4. LAN-first operation; internet/cloud dependency is not required.
5. Never commit API keys, tokens, certificates or private credentials.
6. Prefer hardware decoding/encoding when available.
7. Keep protocol and transport versioned so Windows/macOS hosts can interoperate with Android/iPadOS clients.
8. Keep the IddCx processing loop minimal; network and heavy encoding work belongs outside the driver.
9. TCP is bring-up only; production video uses QUIC datagrams.

## Current engineering state

The repository now contains the major native foundations:
- IddCx driver source foundation;
- experimental GPU shared-resource frame bridge;
- GPU NV12 conversion;
- Media Foundation H.264 encoder path;
- Android MediaCodec renderer;
- TCP bring-up stream;
- protocol/discovery foundation;
- product and adaptive-stream architecture.

These are engineering milestones, not hardware acceptance claims.

## Definition of exploitable

A release is exploitable only when at least one complete path works on real hardware:

**Windows PC -> Android device**

with a real virtual display, production LAN transport, hardware/software decode, stable rendering, configurable resolution/FPS, authenticated pairing, reconnect/keyframe recovery and acceptable latency.

macOS and iPadOS remain separate tracks and will use only supported/public platform APIs.
