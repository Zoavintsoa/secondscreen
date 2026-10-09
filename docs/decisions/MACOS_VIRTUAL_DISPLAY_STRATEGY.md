# macOS Virtual Display Strategy — Independent SecondScreen Implementation

**Status:** Accepted as an engineering direction; implementation and hardware validation are pending.  
**Owner:** Zoavintsoa / SecondScreen  
**Decision branch:** `docs/macos-virtual-display-independent-design`

## Decision

SecondScreen will retain its own architecture, source files, protocol, product identity and implementation history. OpenDisplay is research-only prior art. No OpenDisplay source file, source snippet, copied header, or bundled component is to be imported into SecondScreen as part of this work.

This is **not described as a strict clean-room process**: the project has already inspected OpenDisplay. The accurate claim is that SecondScreen is implementing its own solution after reviewing prior art, with provenance and design decisions documented.

## Why this matters

A real extended desktop requires the operating system to recognize a second display. Capturing and streaming the primary display is screen mirroring, not an extended desktop. ScreenCaptureKit is a supported framework for capturing selected displays, windows and applications, but it does not itself provide a general public API for creating a virtual macOS monitor.

The current macOS boundary in `native/macos/display/MacVirtualDisplay.m` intentionally fails closed rather than pretending that virtual-display creation succeeded. Keep that behavior until a real provider exists.

## Prior-art and provenance policy

- Keep the OpenDisplay fork at `https://github.com/Zoavintsoa/opendisplay` as a separate research repository.
- Do not copy its files, code blocks, private API header, protocol, UI assets, or bundled dependencies into `Zoavintsoa/secondscreen`.
- Record concepts learned from prior art in design notes using original descriptions, not copied implementation text.
- Use official Apple documentation for supported capture APIs and independently verify any undocumented runtime behavior on actual target macOS versions.
- Keep commits small and purpose-specific. Explain design choices and tests in commit messages; retain test results and issue references.
- If future work proposes incorporating third-party source, stop and review its exact license, provenance, notices and distribution obligations before merging. The current OpenDisplay repository is GPL-3.0; this project will not import that code under the present decision.
- Do not claim that Git history proves independent authorship by itself. It is useful evidence of the development process, not a legal guarantee.

## Implementation plan

### Phase 1 — Keep product boundaries honest

1. Keep the virtual-display provider behind the existing macOS display boundary.
2. Distinguish these states in code and UI: unsupported, initializing, active, failed, and stopping.
3. Never report a display as active unless the OS has actually registered it and it can be enumerated as a display.
4. Keep primary-display mirroring available only as an explicitly labelled fallback; never call it an extended desktop.

### Phase 2 — Verify the platform and distribution constraints

1. Confirm the supported macOS versions and minimum host hardware separately from receiver requirements.
2. Investigate Apple's public display-driver route and entitlement requirements before selecting it as a shipping path.
3. If a private CoreGraphics runtime mechanism is prototyped, isolate it in a replaceable provider. Mark it experimental, document that it is unsupported by Apple, and keep it out of any claim of App Store compatibility.
4. Do not reuse OpenDisplay's private header or implementation. Independently document runtime findings, symbols, observed behavior, error cases and OS build numbers.
5. Confirm whether the chosen path can be signed, notarized and distributed under the intended release model before investing in product UI around it.

### Phase 3 — Build a disposable proof of concept

Test on the actual iMac 2019 running macOS 15.8.1 before integrating the implementation into the main stream lifecycle. The proof of concept must demonstrate, in order:

- OS-level registration of a second display;
- visible appearance in macOS display settings / active-display enumeration;
- moving an ordinary application window onto the second desktop;
- capturing that specific display, not the primary display;
- stable resolution and refresh-mode selection;
- clean failure when the runtime API is missing or rejects configuration;
- lifecycle behavior across stop/start and process exit;
- no crash, hang, or false-positive success state.

Record the exact macOS build, hardware, resolution, observed limitations and logs. A successful test on one OS build does not establish compatibility with all macOS versions.

### Phase 4 — Integrate only after proof

1. Define a small provider interface with explicit capability reporting and errors.
2. Connect the real display identifier to the capture pipeline only after OS registration succeeds.
3. Add lifecycle, reconnect and failure tests before UI polish.
4. Preserve the existing Windows + Android first-slice priority; do not let an unvalidated macOS experiment destabilize it.
5. Test older Intel Macs independently. Do not promise MacBook Pro 2013 sender compatibility until its supported OS path is demonstrated.

## Acceptance criteria

The macOS virtual-display feature is not complete until all of these are true:

- macOS recognizes a distinct second display;
- windows can be moved onto it;
- SecondScreen captures the distinct display;
- the client receives and renders its frames;
- failure and teardown paths are verified;
- supported OS/hardware combinations are listed from actual tests;
- the chosen implementation and distribution risks are documented.

Until then, the feature must be described as experimental or not implemented—not as a working virtual extended display.

## References

- Apple ScreenCaptureKit documentation: https://developer.apple.com/documentation/screencapturekit
- Apple Quartz Display Services documentation: https://developer.apple.com/documentation/coregraphics/quartz-display-services
- OpenDisplay research fork (separate repository; no source reuse approved): https://github.com/Zoavintsoa/opendisplay
