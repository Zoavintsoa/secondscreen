# Windows Host

The Windows host is the first production vertical slice.

## Architecture

```
Windows Display Manager
        |
        v
IddCx virtual monitor
        |
        v
D3D11/D3D12 swap-chain frame
        |
        v
SecondScreen frame bridge
        |
        v
Media Foundation H.264/HEVC encoder
        |
        v
LAN transport
```

Microsoft's official IddCx sample is the starting point for the driver because IddCx is the supported Windows model for virtual/remote displays. The production driver must replace the sample's frame-discarding SwapChainProcessor with the SecondScreen frame bridge.

The host executable remains outside the driver. It owns pairing, discovery, session management, encoding, transport, settings and diagnostics.

## Driver requirements

- one or more virtual monitors
- 1920x1080@60 preferred
- 1280x720@60 fallback
- 2560x1440@60 optional
- hot-plug/remove
- correct target mode negotiation
- GPU-backed swap-chain
- frame pacing
- no network API inside the driver
- dedicated UMDF driver process group
- test-signing package for development

Official Microsoft sample:
https://github.com/microsoft/Windows-driver-samples/tree/main/video/IndirectDisplay

## Host requirements

- Windows 10/11 x64
- Media Foundation encoder
- H.264 baseline/main/high profile support
- hardware encoder preferred
- TCP control channel
- TCP video bring-up channel
- UDP/QUIC transport can replace video TCP after functional acceptance
