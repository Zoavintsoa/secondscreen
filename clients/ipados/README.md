# SecondScreen iPadOS client

SecondScreen supports a broad iPadOS range through capability-based fallbacks rather than device-name assumptions.

## Compatibility policy

- Deployment floor: iPadOS 13 for the legacy-compatible branch.
- Modern branch: iPadOS 15+.
- The same protocol and transport model are used across branches.
- H.264 is the baseline codec.
- HEVC, HDR, high-refresh rendering and advanced Pencil behavior are optional capabilities.
- Metal is used when available; the renderer has a conservative fallback path.
- Touch and Pencil input are represented as protocol events and negotiated by capabilities.

## Important build policy

The repository may keep one modern Xcode project while source contains availability-guarded compatibility layers. App Store submission requirements and runtime deployment targets are separate concerns.
