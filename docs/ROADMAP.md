# SecondScreen roadmap

## Phase 1 — Windows + Android vertical slice

- [x] Native repository structure
- [x] Versioned protocol
- [x] LAN discovery foundation
- [x] Pairing manager foundation
- [x] Windows host executable foundation
- [x] Android MediaCodec rendering foundation
- [ ] Official IddCx driver integrated and renamed
- [ ] GPU frame bridge from IddCx swap-chain
- [ ] Media Foundation hardware H.264 encoder
- [ ] QUIC transport with MsQuic
- [ ] Authenticated pairing end-to-end
- [ ] Real virtual display -> Android video path
- [ ] Reconnect + forced keyframe recovery
- [ ] Touch/stylus return channel
- [ ] Adaptive bitrate/FPS

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

## Phase 3 — macOS + iPadOS

- [ ] Intel macOS host
- [ ] Apple Silicon host
- [ ] Virtual display implementation based only on supported Apple APIs
- [ ] ScreenCaptureKit capture
- [ ] VideoToolbox encode
- [ ] iPadOS client
- [ ] Metal renderer
- [ ] Apple Pencil/touch input

## Phase 4 — Advanced

- [ ] Multiple simultaneous clients
- [ ] Multiple virtual monitors
- [ ] Audio transport
- [ ] USB/HID extensions
- [ ] Network-quality adaptive profiles

### Rule

A checkbox is not marked complete because code exists. It is complete only after the corresponding real-hardware acceptance test is passed.
