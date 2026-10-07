# Open-source engineering research

Updated: 2026-10-08

## Objective

SecondScreen is a native local-first virtual-display platform. The reference target is not generic remote desktop: the host must create a real virtual display, encode frames with the GPU/video stack, transport them with low latency, decode them natively on Android/iPadOS, and later expose touch/stylus and creator workflows.

We use open-source projects as engineering references or source components only when their licenses are compatible and attribution obligations can be satisfied.

## Best references

| Project | Best contribution | License | Decision |
|---|---|---|---|
| Microsoft Windows Driver Samples / IddSampleDriver | Windows IddCx virtual-display foundation, lifecycle and frame callbacks | MS-PL | **Use as driver foundation**, preserve notices |
| VirtualDrivers/Virtual-Display-Driver | Production-oriented IddCx virtual-monitor details, EDID and monitor identity patterns | MIT | **Study and selectively adapt** |
| Microsoft MsQuic | QUIC 1-RTT/TLS, reliable streams, unreliable datagrams, low-latency transport | MIT | **Primary production transport** |
| LizardByte Sunshine | Hardware capture/encode architecture, low-latency streaming and client pairing ideas | GPL-3.0 | **Architecture reference only; do not copy GPL code into the core** |
| Moonlight Android | Android native streaming/decoder/input architecture | GPL-3.0 | **Architecture/reference only** |
| Tether | Modern cross-platform low-latency pipeline, codec capability probing and transport abstraction | MIT | **Study/adapt compatible ideas** |
| Orbiscreen | Secondary-display-specific protocol, H.264 framing, Android client, stylus/touch concepts | GPL-3.0 | **Architecture reference only** |

## Our resulting architecture

1. Virtual display: IddCx on Windows. Keep the driver minimal and move encode/network work out of the driver.
2. Frame path: shared GPU texture -> GPU color conversion -> hardware H.264/HEVC -> access unit.
3. Transport: MsQuic. One reliable bidirectional SSCP control stream plus SSVG QUIC datagrams for video.
4. Recovery: fragment loss invalidates an access unit; client requests an IDR/keyframe rather than attempting unsafe partial decode.
5. Security: TLS protects QUIC; the six-digit pairing code is only user confirmation. A separate random session credential authorizes the device.
6. Android: keep QUIC behind an adapter. Native MediaCodec remains the decode boundary.
7. Smart Stream: integrate only real RTT/loss/jitter/encode/decode/present telemetry. Never invent measurements.
8. Apple: supported public APIs only; ScreenCaptureKit/VideoToolbox/Metal, with no private virtual-display API assumption.

## License policy

- MIT and MS-PL components may be incorporated when their notices and license conditions are retained.
- GPL/AGPL code is not copied into the SecondScreen core merely because it is technically useful. We can reproduce public architectural ideas and reimplement compatible behavior.
- Third-party source files must keep provenance in a THIRD_PARTY_NOTICES.md file before distribution.
- No dependency is accepted only because it is popular; it must improve one of the product-critical paths.

## Priority decision

The research confirms the current roadmap instead of expanding it:

IddCx -> GPU frame handoff -> hardware encode -> MsQuic -> authenticated control -> QUIC video -> Android native decode -> keyframe recovery -> adaptive engine -> input/stylus -> creator roles.

Everything else is secondary until the Windows-to-Android vertical slice is real and hardware-validated.
