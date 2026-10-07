# macOS Host

Reserved for the second host implementation.

Target:
- Intel macOS support first (the user's iMac 2019 is Intel).
- supported virtual-display API only; no undocumented/private display API.
- ScreenCaptureKit for capture where appropriate.
- VideoToolbox for hardware H.264/HEVC.
- MsQuic transport shared with Windows.
- explicit Screen Recording and accessibility/input permissions.

The macOS virtual-display mechanism will be selected after verifying the currently supported Apple APIs. No fake virtual monitor implementation will be accepted.
