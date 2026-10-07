# SecondScreen Android client

SecondScreen targets a broad Android device range, not a single handset.

## Compatibility policy

- Minimum API: 26 (Android 8.0).
- Target/compile API: 36.
- Runtime capability detection is mandatory.
- Hardware decoder, resolution, refresh rate, HDR, input and QUIC features are negotiated at runtime.
- No feature may assume a specific GPU, codec vendor or Android release.
- Unsupported capabilities degrade to the next compatible mode instead of blocking connection.

### Transport

The Android client exposes a transport-neutral QUIC adapter. The production implementation may use a native MsQuic/JNI backend or another compatible QUIC backend without changing the display, control or video layers.

### Video

H.264 is the baseline codec. HEVC is optional and enabled only after capability probing. The client uses MediaCodec and bounded SSVG reassembly. Lost/incomplete access units trigger keyframe recovery.

### Support tiers

- Tier A: Android 10+ with hardware H.264 decoder and modern networking.
- Tier B: Android 8/9 devices meeting the baseline MediaCodec/network requirements.
- Tier C: devices with incomplete codec/network capabilities; safe profile only, with diagnostics explaining limitations.

Android 8.0/API 26 is a compatibility floor, not a promise that every device can sustain 1080p60.
