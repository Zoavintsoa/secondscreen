# SecondScreen — Android Client Architecture & Build Guide

## 1. Overview
The Android Client subsystem provides the low-latency hardware decoding receiver and immersive touch digitizer for SecondScreen.

### Key Components:
- **`MediaCodecDecoder.kt`**: Hardware-accelerated H.264/AVC decoding with `COLOR_FormatSurface` and `KEY_LOW_LATENCY`.
- **`SurfaceRenderView.kt`**: Zero-copy display surface overlaying the Android hardware compositor.
- **`SecondScreenClient.kt`**: Asynchronous coroutine-based socket receiver with automatic reconnection and round-trip ping/pong tracking.
- **`MainActivity.kt`**: Immersive sticky fullscreen mode hiding all navigation/status bars.

---

## 2. Build & Deployment
```bash
cd native/android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```
