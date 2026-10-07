# SecondScreen Architecture

## Product

SecondScreen is a virtual-display product, not a web page.

### End-to-end pipeline

1. Host registers a virtual monitor with the operating system.
2. OS renders desktop applications onto that monitor.
3. Host captures the virtual-monitor framebuffer.
4. Host encodes frames using GPU hardware where possible.
5. Host sends a low-latency stream over the local network.
6. Client decodes frames with hardware acceleration.
7. Client displays the frames fullscreen.
8. Client input is optionally returned to the host.

## First vertical slice

Windows + Android is the priority because it gives the fastest route to a practical spacedesk-like product.

## Performance targets

- 1080p minimum
- 30 FPS minimum
- 60 FPS target
- LAN operation
- low startup time
- automatic reconnect
- configurable bitrate
- configurable resolution/FPS
- visible latency/connection diagnostics

## Security

Pairing must establish a per-device session credential. Never embed a permanent secret in the client binary or source repository.


## Transport decision

Production transport is **MsQuic** rather than WebRTC. The product is a LAN-first remote-display protocol, not a browser media call. QUIC gives us encrypted/authenticated transport, reliable control streams, unreliable datagrams for video, reconnect/path migration, and one protocol shared by Windows and Android. TCP remains the bring-up fallback only.

## Frame pipeline decision

The critical path is GPU-backed end-to-end:

IddCx D3D surface -> GPU encoder -> compressed access unit -> QUIC datagram -> MediaCodec surface decoder.

CPU readback is a fallback/debug path, not the normal path. This avoids copying 1080p/60 frames through system memory.

## Product layers

- **Driver:** only virtual-display lifecycle, modes, swap-chain acquisition and GPU frame handoff.
- **Host service:** discovery, pairing, session security, encoding, QUIC transport, settings, telemetry.
- **Client:** discovery, pairing, MediaCodec decode, SurfaceView rendering, input return.
- **Protocol:** versioned, platform-neutral wire contract.

This separation prevents a network failure from destabilizing the Windows display driver and allows the Android/iPad clients to evolve independently.

## Quality ladder

1. Real virtual display.
2. Real frame acquisition.
3. Real hardware/software H.264 encode.
4. Real LAN transport.
5. Real hardware decode/render.
6. Reconnect + keyframe recovery.
7. Pairing/security.
8. Adaptive bitrate/FPS and latency telemetry.
9. Touch/stylus input.
10. Multi-monitor and 1440p/4K modes.
11. macOS host and iPadOS client.

Nothing in this ladder is marked complete from simulation alone.
