# SecondScreen Android Client

Native Kotlin client for the SecondScreen LAN display protocol.

## Current pipeline

LAN H.264/HEVC access unit -> MediaCodec -> SurfaceView.

The decoder deliberately refuses to invent frames. Discovery, authenticated pairing and the final host endpoint are separate layers.

## Target

Android 8+ client, tested first on Android 14 / ZTE Z2453.

## Next production layers

1. UDP/mDNS host discovery.
2. Pairing code and per-device session token.
3. Stream configuration from host instead of fixed 1920x1080.
4. H.264 decoder capability selection.
5. Keyframe request on reconnect.
6. Touch/stylus input return channel.
7. Network statistics and latency display.
