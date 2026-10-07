# Android QUIC transport

SecondScreen keeps QUIC behind AndroidQuicTransport.

The selected production candidate is Microsoft MsQuic. MsQuic documents Android as a supported platform and its source is MIT licensed. The Android build uses the upstream CMake/NDK path rather than copying GPL application code.

## Current state

- Kotlin transport boundary: implemented.
- JNI ABI/library loading: implemented.
- Safe native stub: implemented; it reports unavailable rather than pretending TCP is QUIC.
- TCP remains the explicit compatibility/bring-up path.
- Upstream MsQuic Android integration is isolated from display, control and decoder code.

## Security

QUIC TLS identity is never derived from the six-digit pairing code. The pairing code confirms user intent; the host-issued session credential authenticates the application session.

## Compatibility

The Android application floor remains API 26. QUIC availability is capability-driven: if the native QUIC ABI is unavailable, the client uses the explicit TCP bring-up path rather than claiming production QUIC.
