# Native SecondScreen

This is the implementation area for the real SecondScreen product.

## Milestone 1: Windows Host + Android Client

The first exploitable target is:

Windows PC -> real virtual secondary display -> LAN -> Android device.

The host must expose an actual OS display through Windows Indirect Display Driver (IddCx), capture its frames, encode them, and stream them over the local network. The Android client must discover/pair with the host, decode frames with MediaCodec, render fullscreen, and report connection state.

## Required acceptance tests

- Virtual display appears in Windows Display Settings.
- Resolution and refresh rate can be changed.
- Android client discovers host on LAN.
- Pairing is authenticated.
- Video is visible continuously on the Android device.
- Hardware decoding is used when supported.
- Disconnect/reconnect recovers without restarting the PC.
- No cloud service is required.
- No API key is required for normal LAN operation.

## Platform directories

- windows-host/
- macos-host/
- android-client/
- ipados-client/
- shared-protocol/
