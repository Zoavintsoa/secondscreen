# Smart Stream Engine

## Purpose

The Smart Stream Engine is the local control loop that selects the best stream profile for the current device, scene and network conditions.

It must not require an internet service.

## State

The controller tracks:
- current profile;
- target profile;
- current bitrate;
- current FPS;
- current resolution;
- RTT;
- jitter;
- loss ratio;
- encode time;
- decode time;
- presented FPS;
- queue depth;
- thermal/battery hints;
- time since last profile change.

## Derived signals

### Network pressure

Increase pressure when:
- RTT rises;
- jitter rises;
- packet loss persists;
- send queue grows.

### Compute pressure

Increase pressure when:
- encode time approaches frame duration;
- decode time approaches frame duration;
- queue depth grows;
- presented FPS falls below target.

### Scene activity

Classify roughly as:
- static;
- desktop motion;
- video;
- high-motion/game.

The classifier may start with frame-difference statistics and later use GPU-side metrics.

## Hysteresis

The controller must have:
- fast degradation;
- slow recovery;
- minimum dwell time;
- consecutive-sample thresholds.

Example policy:

| Condition | Action |
|---|---|
| sustained congestion | lower bitrate |
| congestion persists | lower FPS |
| latency remains high | lower resolution |
| stable path | recover bitrate slowly |
| decoder reset | pause deltas and request keyframe |
| profile change | request keyframe |

No single noisy sample should trigger a profile change.

## Initial profile ladder

| Profile | Resolution | FPS | Target bitrate |
|---|---:|---:|---:|
| Safe | 1280x720 | 30 | 4 Mbps |
| Balanced | 1920x1080 | 60 | 8 Mbps |
| Quality | 2560x1440 | 60 | 16 Mbps |
| Ultra | 3840x2160 | 60 | 30 Mbps |

These values are starting points, not guarantees.

## Creator behavior

Creator profiles bias toward stable frame cadence and predictable latency.

For a static UI, the controller should eventually allow a lower capture/encode cadence while preserving responsiveness to changes. For video playback or camera monitoring, it should favor stable real-time FPS.

## Future control surface

The engine will expose a local diagnostics model:
- quality score;
- current profile;
- bitrate;
- FPS;
- resolution;
- RTT;
- loss;
- jitter;
- encode/decode time;
- dropped frames;
- recovery count.

This model will feed the Windows tray UI and Android/iPad diagnostics without changing the wire protocol.
