# SecondScreen — Product Vision

## Positioning

SecondScreen is not a browser-based remote desktop and not only a second-monitor utility.

It is a local device-to-workspace platform:

> Your devices become your workspace.

The core product remains a real operating-system virtual display. The differentiating layer is that the client device can take a deliberate role in the user's workflow.

## Core modes

### Display
A normal secondary monitor with fullscreen, orientation and resolution profiles.

### Creator
Optimized presets for photography, video and live production:
- clean preview monitor;
- scopes/monitoring surface;
- timeline/transport surface;
- production control surface;
- teleprompter;
- touch shortcut surface.

### Camera Monitor
A client can become a production monitor fed through the host:
- fullscreen preview;
- waveform/vectorscope/false-color extensions;
- focus and exposure assistance;
- low-latency monitoring;
- portrait/landscape profiles.

### Tablet
Touch and stylus become authenticated input devices:
- pressure;
- tilt where available;
- buttons/modifiers;
- normalized coordinates;
- low-latency return channel.

### Gaming
A latency-first profile:
- short queues;
- aggressive congestion response;
- high refresh targets where the client and network allow it;
- host scheduling priority hints.

## Smart Stream Engine

The stream controller must optimize for perceived latency and usability, not bitrate alone.

Inputs:
- capture FPS;
- encode time;
- encode queue depth;
- client decode time;
- presented FPS;
- RTT;
- packet loss;
- jitter;
- device thermal state where available;
- battery state;
- client native resolution;
- orientation;
- scene activity.

Policy:
1. Keep the display responsive.
2. Reduce bitrate before reducing FPS.
3. Reduce FPS before reducing resolution.
4. Reduce resolution when latency remains above target.
5. Increase quality slowly after a stable period.
6. Force a keyframe after a profile change or decoder recovery.
7. Never oscillate rapidly between profiles.

The engine is deterministic and local. No cloud service is required.

## Zero-configuration pairing

Normal flow:

1. host appears automatically on LAN;
2. client selects host;
3. host displays a short-lived six-digit code;
4. user confirms the code;
5. a random per-device session credential is created;
6. subsequent connections use the stored credential;
7. the user can revoke a device at any time.

The pairing code is never a permanent password.

## Device roles

The same protocol supports different client roles without changing the Windows display core:

- Android phone/tablet: monitor, touch control, scopes, camera monitor;
- iPad: monitor, Pencil tablet, timeline/control surface;
- future client: another computer or embedded display.

## Profiles

Profiles are declarative and versioned:

- display_balanced
- creator_preview
- creator_scopes
- camera_monitor
- tablet_input
- gaming_low_latency

A profile selects:
- resolution;
- FPS;
- bitrate range;
- keyframe interval;
- latency target;
- color mode;
- input capabilities;
- optional role-specific extensions.

## Privacy and trust

- LAN-first.
- No mandatory account.
- No mandatory cloud relay.
- QUIC/TLS for production transport.
- Per-device credentials.
- Revocation.
- Authenticated input only.
- Diagnostics are local by default.

## Architectural principle

Keep the virtual-display path boring and dependable.

Innovation belongs above it:
- stream intelligence;
- device roles;
- creator workflows;
- diagnostics;
- input;
- profiles;
- extensions.

This prevents product features from destabilizing the display driver.
