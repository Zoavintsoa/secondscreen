# SecondScreen — Transport Architecture

## Goal

Keep video, control, pairing and recovery independent from the physical transport.

### Canonical layers

Host capture → hardware encoder → SSVF/SSVG video framing → Transport → client reassembler/decoder

The transport must never parse H.264 beyond framing boundaries.

## Transport matrix

| Host | Android | iPadOS |
|---|---|---|
| macOS | Wi-Fi/TCP/QUIC + AOA USB | Wi-Fi/TCP/QUIC |
| Windows | Wi-Fi/TCP/QUIC + AOA USB | Wi-Fi/TCP/QUIC |

### Android USB

macOS/Windows are USB hosts and Android uses Android Open Accessory (AOA).

Flow:
1. Detect Android USB device.
2. Query AOA protocol.
3. Send manufacturer/model/version/URI.
4. Start accessory mode.
5. Wait for AOA re-enumeration.
6. Open the accessory bulk endpoints.
7. Send complete SSVF frames.
8. Android receives through UsbAccessory.
9. Decoder waits for a keyframe after connect/reconnect.

AOA is the canonical wired Android transport.

### iPadOS USB

A direct generic PC ↔ iPad application transport is **not** implemented as a raw USB stream.

Apple's ExternalAccessory framework is intended for supported MFi accessories, and iPadOS DriverKit USB support is for compatible iPad hardware/drivers rather than a generic PC application-to-app USB pipe.

Therefore the product architecture exposes USB as an optional future Apple-accessory backend, but the supported zero-hardware-add-on wired path for iPad is LAN/Wi-Fi. We must not ship a private-API or jailbreak-dependent solution.

## Windows Android USB backend

The Windows host uses the same AOA protocol as macOS:
- WinUSB/USB device enumeration
- AOA protocol request
- accessory start/re-enumeration
- bulk OUT for SSVF video
- optional bulk IN for control/input in a later phase

No ADB dependency is required for production.

## Mode policy

AUTO:
1. USB wired transport when a supported USB accessory is present.
2. Otherwise QUIC when available and authenticated.
3. Otherwise TCP LAN bring-up.

USB:
- Never silently fall back to LAN.
- Retry until cable/device is available.
- Reset decoder and require a keyframe after every reconnect.

WIFI/LAN:
- Never attempt USB.
- Use the existing discovery + authenticated control + video transport.

## Recovery contract

Every transport must provide:
- connected/disconnected state
- ordered or framed byte delivery
- bounded frame size
- cancellation
- reconnect
- keyframe recovery notification

The video layer owns:
- SSVF validation
- keyframe gating
- decoder recovery

The transport layer owns:
- USB/TCP/QUIC I/O
- endpoint discovery
- connection lifetime
- backpressure

## Security

AOA itself is a physical transport, not an authentication mechanism.

Production USB sessions must still use the same application-level pairing/session protocol before accepting control/input. Test-mode video bridges are not production-secure.

## Test matrix

### macOS → Android
- Wi-Fi discovery
- TCP video
- USB AOA video
- disconnect/reconnect
- keyframe recovery
- latency comparison

### Windows → Android
- USB AOA
- Wi-Fi
- reconnect
- keyframe recovery

### macOS → iPadOS
- Wi-Fi
- TCP/QUIC
- decoder and rendering
- reconnect

### Windows → iPadOS
- Wi-Fi
- TCP/QUIC
- decoder and rendering
- reconnect

### iPad USB future
Requires a supported Apple accessory/MFi or another Apple-approved transport. Do not substitute private USB APIs.
