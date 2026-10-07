# SecondScreen Roadmap

## Phase 1 — Web engineering console
- [x] React/Vite application
- [x] Host dashboard
- [x] Client view
- [x] Protocol model
- [x] Diagnostics matrix
- [x] DaVinci-oriented dual-screen simulator
- [x] Browser screen-capture prototype
- [ ] Production LAN transport

## Phase 2 — Windows Host
- [ ] Real IddCx virtual display driver project
- [ ] Driver INF/package
- [ ] DXGI capture pipeline
- [ ] Hardware H.264/HEVC encoder
- [ ] Native TCP/UDP transport
- [ ] Pairing and session authentication
- [ ] Installer and signed test package

## Phase 3 — Android Client
- [ ] Android Studio project
- [ ] Pairing UI
- [ ] MediaCodec decoder
- [ ] SurfaceView renderer
- [ ] Fullscreen/rotation
- [ ] Touch/stylus return channel
- [ ] Physical LAN validation

## Phase 4 — macOS Host
- [ ] Native display implementation
- [ ] ScreenCaptureKit capture
- [ ] VideoToolbox encoder
- [ ] Network transport
- [ ] Screen Recording/TCC handling
- [ ] Physical validation on Intel and Apple Silicon

## Phase 5 — iPadOS Client
- [ ] Swift client
- [ ] VideoToolbox decoder
- [ ] Metal renderer
- [ ] Pencil/touch input
- [ ] Physical validation

## Validation rule

The UI must distinguish between **simulation**, **prototype**, **implemented/untested**, and **physically validated**. No native feature should be marked as end-to-end validated until it has been tested on the target OS and hardware.
