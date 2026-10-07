# Workspace Profiles

Profiles turn SecondScreen from a generic display into a workflow device.

## Initial profiles

### Display Balanced
General-purpose second monitor.

### Creator Preview
Large clean preview, stable cadence, moderate latency target.

### Creator Scopes
Prioritize predictable frame cadence and a monitoring-oriented client role.

### Camera Monitor
Prioritize latency, stable FPS and monitoring overlays.

### Tablet Input
Prioritize touch/stylus return latency and display responsiveness.

### Gaming Low Latency
Minimize buffering and react quickly to congestion.

## Profile format

Profiles are versioned local data. A profile contains:
- id;
- schemaVersion;
- resolution;
- FPS;
- bitrateMinKbps;
- bitrateTargetKbps;
- bitrateMaxKbps;
- keyframeInterval;
- latencyTargetMs;
- colorSpace;
- rotation;
- inputMode;
- role.

The host remains authoritative for negotiated display parameters.

## Extension model

Role-specific features should be extensions above the core:
- scopes;
- camera-monitor;
- timeline-control;
- teleprompter;
- shortcut-deck;
- photoshop-tablet;
- davinci-control;
- obs-control;
- vmix-control.

An extension may consume telemetry and input, but it must not bypass authentication or the core transport.
