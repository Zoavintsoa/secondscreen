# SecondScreen — visual identity

## Product lockup
**SecondScreen — Zoavintsoa** is the product name. Keep the creator attribution visible in the desktop dashboard, mobile app label, About screen and documentation.

## Design tokens
- **Background:** `#101722` (deep blue-black)
- **Panel:** `#1A1F29` (raised surface)
- **Accent:** `#20C7F4` (cyan)
- **Primary text:** `#F3F8FF`
- **Secondary text:** muted neutral gray
- **Typography:** native system sans-serif; monospaced uppercase labels for technical metadata
- **Shape language:** rounded panels, restrained borders, clear focus states, generous spacing

## Icon concept
A pair of overlapping displays represents the core product: one host display and one connected screen. The cyan accent signals the active connection and technical workspace. The current Android vector icon and macOS SF Symbol are first-pass native assets; they are not a final marketing logo.

## Interface principles
1. Preserve the native platform controls and accessibility support.
2. Keep connection state and the active display mode visible.
3. Disable unavailable modes instead of implying they work.
4. Label prototype, experimental and hardware-validated features accurately.
5. Keep video playback uncluttered; controls may be hidden or minimized when the client is showing a stream.
6. Use the same color tokens and product naming across platforms, adapting layouts to each platform.

## Feature state labels
- **Disponible** — implemented and verified for the stated path.
- **Expérimental** — implementation exists but reliability or hardware coverage is incomplete.
- **En développement** — interface or architecture groundwork exists; the feature is not yet usable end to end.
- **Indisponible** — blocked by platform/API/hardware limitations.

The existence of an interface, protocol, driver scaffold or CI build is not proof of end-to-end hardware support.
