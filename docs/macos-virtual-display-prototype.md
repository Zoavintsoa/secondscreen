# macOS Virtual Display Prototype

**Status: experimental, not production-ready.** This proof of concept isolates virtual-display creation behind `SecondScreenMacVirtualDisplayManager` and uses undocumented CoreGraphics runtime classes. Apple’s supported ScreenCaptureKit API captures existing displays; it does not create a new OS display. See Apple’s [ScreenCaptureKit documentation](https://developer.apple.com/documentation/screencapturekit/capturing-screen-content-in-macos).

## Included

- A manager with input validation and explicit failure messages.
- Native macOS host project integration.
- UI controls to create a 1920×1080, 60 Hz monitor and request its removal.
- Explicit status that the existing Android test stream still captures the primary display.

## Limitations

This uses undocumented Apple implementation details and may stop working after a macOS update. It is a local development experiment, not suitable for claiming production support or Mac App Store readiness. The create button alone does not prove success: the monitor must appear in **System Settings → Displays** and accept a normal application window.

Releasing the private display object may not immediately remove the monitor from the OS display topology on every macOS build. The **Remove monitor** button is best-effort only; if the monitor remains, quit the application and verify that macOS removes it. Do not rely on repeated create/remove cycles in one process until the target Mac has confirmed that lifecycle.

## Hardware validation

1. Build the `SecondScreenMacHost` target in Xcode on the test Mac.
2. Select **Créer le moniteur virtuel**.
3. Record any error message and Console logs if it fails.
4. If creation reports success, verify a distinct display in **Réglages Système → Moniteurs** and move a normal window onto it.
5. Select **Supprimer le moniteur** and check whether the display disappears. If not, quit SecondScreen and verify again.
6. Restart the app before attempting another create/remove cycle.
7. Record the macOS build, display ID, resolution, and refresh rate.

## Next stage

After lifecycle validation on physical hardware, update the capture backend to select the new display ID. Then validate capture, H.264 encoding, LAN transport, and client decoding separately. Until then, describe the Mac-to-client stream as a primary-display test stream.
