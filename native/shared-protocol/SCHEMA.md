# Protocol schema

Canonical message names and fields live in `protocol/PROTOCOL.md`.

Implementation rule:
- Windows, macOS, Android and iPadOS must share the same major protocol version.
- Minor versions are backward compatible.
- Unsupported optional capabilities are negotiated rather than guessed.
- Every video stream starts with codec configuration data followed by a keyframe.
- A reconnect must not display stale pre-disconnect frames.
