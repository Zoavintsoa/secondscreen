# SecondScreen - Android Client Native Architecture & Build Guide

## 1. Overview
The Android Client component provides the low-latency hardware decoding receiver and immersive touch digitizer for SecondScreen.

### Key Technologies:
- **Language**: Kotlin 1.9+ / Java 17
- **Hardware Decoder**: Android `MediaCodec` (H.264 / AVC) configured with `KEY_LOW_LATENCY` (Android 11+) and `COLOR_FormatSurface`
- **Zero-Copy Rendering**: `SurfaceView` overlaying hardware compositor without CPU bitmap conversion
- **Networking**: Kotlin Coroutines with TCP Control Socket & Binary Frame Stream receiver
- **Input Forwarding**: Normalized coordinate digitizer supporting multi-touch and stylus pressure

---

## 2. Directory Layout
```
native/android/
├── build.gradle.kts                      # Root Gradle configuration
├── settings.gradle.kts                   # Project module includes
└── app/
    ├── build.gradle.kts                  # App module dependencies & SDK levels
    └── src/main/
        ├── AndroidManifest.xml           # Permissions & full-screen activity
        └── java/com/secondscreen/client/
            ├── MainActivity.kt           # Immersive UI & pairing coordinator
            ├── decoder/
            │   └── MediaCodecDecoder.kt  # Hardware H.264 NALU decoder -> Surface
            ├── network/
            │   └── SecondScreenClient.kt # Asynchronous socket client & protocol state machine
            └── ui/
                └── SurfaceRenderView.kt  # Touch & Stylus digitizer
```

---

## 3. Building the Android Client

### Prerequisites:
- Android Studio (Flamingo / Hedgehog / Koala or newer)
- Android SDK 34 (Android 14) with minimum SDK 26 (Android 8.0 Oreo)
- Physical Android Tablet (e.g. Samsung Galaxy Tab S7/S8/S9, Xiaomi Pad, Lenovo Tab)

### Build Commands:
```bash
cd native/android
./gradlew assembleDebug
```
Deploy to connected tablet via ADB:
```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

---

## 4. Hardware Verification Procedure
1. Ensure both the Windows Host PC and Android Tablet are connected to the same 5GHz Wi-Fi LAN or USB-tethered network.
2. Launch the SecondScreen Host on Windows.
3. Open SecondScreen on Android; the host will be discovered via UDP broadcast (or enter IP manually).
4. Enter the 6-digit PIN shown on the Windows Host.
5. The Android device will enter immersive fullscreen mode and render the live Display 2 stream.
