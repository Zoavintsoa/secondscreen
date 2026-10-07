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
