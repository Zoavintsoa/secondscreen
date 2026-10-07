# SecondScreen Architecture

## Product

SecondScreen is a virtual-display product, not a web page.

It is also a local workspace platform: the virtual display is the dependable core, while smart streaming, device roles, input and creator profiles differentiate the product.

### End-to-end pipeline

1. Host registers a virtual monitor with the operating system.
2. OS renders desktop applications onto that monitor.
3. IddCx supplies GPU-backed swap-chain frames.
4. A minimal GPU frame bridge hands frames to the host service.
5. Host converts/encodes frames using GPU hardware where possible.
6. Host sends low-latency video over the production QUIC datagram path.
7. Client decodes with hardware acceleration.
8. Client displays fullscreen.
9. Optional authenticated input returns to the host.
10. Smart Stream Engine continuously adjusts quality from measured conditions.

## First vertical slice

Windows + Android is the priority.

The current implementation contains a native Windows IddCx foundation, an experimental GPU shared-resource bridge, a Media Foundation H.264 path, a TCP bring-up transport and an Android MediaCodec client. These components are engineering foundations and are **not yet hardware-accepted**.

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
- target end-to-end latency below 50 ms on a strong LAN

## Security

Pairing must establish a per-device session credential. Never embed a permanent secret in the client binary or source repository.

Discovery is metadata only. Input is disabled until authentication succeeds.

## Transport

Production transport is **MsQuic**:
- reliable bidirectional stream for control/auth/configuration/input/telemetry;
- QUIC datagrams for video;
- TCP 49153 is temporary bring-up only;
- UDP discovery is 49151;
- the future control endpoint is 49152.

The transport boundary is intentionally separate from display and encoding code.

## Frame pipeline

Preferred path:

IddCx D3D surface -> asynchronous GPU frame bridge -> GPU color conversion -> hardware H.264/HEVC encoder -> QUIC datagram -> MediaCodec surface decoder.

CPU readback is a fallback/debug path, not the normal path.

The IddCx frame-processing loop must remain minimal. Significant work belongs outside the driver callback/processing loop.

## Product layers

- **Driver:** virtual-display lifecycle, modes, swap-chain acquisition and minimal GPU frame publication.
- **Host service:** discovery, pairing, authentication, encoding, transport, Smart Stream Engine, settings and telemetry.
- **Client:** discovery, authentication, hardware decode, rendering, diagnostics and authenticated input.
- **Protocol:** versioned, platform-neutral wire contract.
- **Extensions:** creator roles, scopes, camera monitor, timeline/control surfaces and future integrations.

This separation prevents network failure from destabilizing the Windows display driver.

## Quality ladder

1. Real virtual display.
2. Real frame acquisition.
3. Real GPU frame bridge.
4. Real hardware/software H.264 encode.
5. Real LAN transport.
6. Real hardware decode/render.
7. Reconnect + keyframe recovery.
8. Authenticated pairing.
9. Adaptive bitrate/FPS and latency telemetry.
10. Touch/stylus input.
11. Multi-monitor and 1440p/4K modes.
12. Creator profiles/extensions.
13. macOS host and iPadOS client.

Nothing in this ladder is marked complete from simulation alone.
