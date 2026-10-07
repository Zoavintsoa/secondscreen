# SecondScreen

SecondScreen is a native, cross-platform alternative to spacedesk: a computer host creates a virtual secondary display and streams that display over the local network to an Android tablet, iPad, or another supported client.

## Product goal

**Host computer → virtual display → low-latency LAN stream → client device**

The web UI that existed in this repository was only an engineering prototype. It is not the product and must not be treated as the finished application.

## Native architecture

### Windows host
- Windows Indirect Display Driver (IddCx)
- DXGI desktop capture
- hardware H.264/HEVC encoding
- TCP/UDP or QUIC transport
- pairing/authentication
- display mode and resolution management

### macOS host
- virtual display implementation
- ScreenCaptureKit
- VideoToolbox hardware encoding
- LAN transport
- Screen Recording/TCC permissions

### Android client
- Kotlin
- MediaCodec hardware decoder
- SurfaceView/TextureView renderer
- fullscreen and rotation
- touch/stylus input channel
- automatic host discovery and pairing

### iPadOS client
- Swift
- VideoToolbox
- Metal rendering
- touch/Pencil input

## Engineering rules

1. Native functionality takes priority over UI mockups.
2. No feature is considered complete until it works on real hardware.
3. Simulation/prototype code must never be presented as a real virtual display.
4. LAN-first operation; internet/cloud dependency is not required.
5. Never commit API keys, tokens, certificates or private credentials.
6. Prefer hardware decoding/encoding when available.
7. Keep protocol and transport versioned so Windows/macOS hosts can interoperate with Android/iPadOS clients.

## Target repository structure

```
native/
  windows-host/
  macos-host/
clients/
  android/
  ipados/
protocol/
docs/
tests/
```

## Current state

The repository is being migrated from the original React engineering prototype toward the actual native SecondScreen application. Existing protocol/reference code may be reused when technically sound, but native implementation is the acceptance target.

## Definition of exploitable

A release is exploitable only when at least one complete path works:

**Windows PC → Android device**

with a real virtual display, real LAN transport, hardware/software decode, stable rendering, configurable resolution/FPS, pairing/security, and recovery from disconnect/reconnect.

The next milestone is therefore the Windows Host + Android Client vertical slice.
