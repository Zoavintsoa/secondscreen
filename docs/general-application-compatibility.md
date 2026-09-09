# SecondScreen — General-Purpose Multi-Monitor Application Compatibility

## 1. Principle of OS-Level Virtual Display
SecondScreen operates at the operating system driver level. Because Windows and macOS recognize `Display 2` as a hardware display in System Settings / System Preferences, **any desktop application that supports multiple monitors works out-of-the-box without plugins, extensions, or application-specific integrations**.

The tablet or iPad functions identically to an external HDMI / DisplayPort monitor connected to your workstation.

---

## 2. General-Purpose Desktop Application Test Suite

The following desktop applications have standardized multi-window workflows that can be moved directly to Display 2:

### A. Video & Color Grading
- **DaVinci Resolve Studio**:
  - Move **Video Scopes** (RGB Parade, Waveform, Vectorscope, Histogram) to Display 2.
  - Undock **Fairlight Audio Mixer** or **Inspector** to Display 2.
- **Adobe Premiere Pro**:
  - Undock and move **Program Monitor**, **Lumetri Color**, or **Audio Track Mixer** to Display 2.
- **Adobe After Effects**:
  - Place **Composition Viewer** or **Timeline Preview** on Display 2.

### B. Audio Production & DAWs
- **Steinberg Cubase / Nuendo**:
  - Move **MixConsole (F3)**, **Channel Settings**, or VST instrument/effect plugin windows to Display 2.
- **Image-Line FL Studio**:
  - Undock and move **Mixer**, **Piano Roll**, or **Mastering VSTs** to Display 2.
- **Ableton Live**:
  - Enable **Dual Screen Mode** (`View → Second Window`) to place Session View or Clip Detail on Display 2.
- **Avid Pro Tools**:
  - Move **Mix Window** to Display 2 while maintaining Edit Window on Display 1.

### C. Creative & 3D Design
- **Adobe Photoshop / Illustrator**:
  - Move floating tool panels, Color Picker, Layers palette, and Navigator to Display 2.
- **Blender**:
  - Open a **New Main Window** (`Window → New Main Window`) and drag it to Display 2 as a dedicated 3D Viewport, Shader Editor, or Timeline.
- **Unreal Engine 5**:
  - Undock Blueprint editors, Material graph editors, or Viewport to Display 2.

### D. Live Streaming & Broadcast
- **OBS Studio**:
  - Enable **MultiView (Full Screen)** on Display 2 (`View → Multiview (Fullscreen) → Display 2`).
  - Undock audio mixers, Twitch chat, and stream stats to Display 2.
- **vMix**:
  - Route **Fullscreen Output 1/2** directly to SecondScreen Virtual Display.

### E. Productivity & Web
- **Google Chrome / Microsoft Edge / Safari**:
  - Drag browser tabs or developer tools to Display 2.
- **Microsoft Office / Excel / Word / PowerPoint**:
  - Presenter View on Display 1, full-screen slides on Display 2.

---

## 3. Physical Validation Checklist for General Desktop Apps

1. Confirm **Display 2** is active in Windows Display Settings / macOS Displays.
2. Launch the target application (e.g. Photoshop, Cubase, OBS Studio).
3. Drag the target window or panel across the desktop boundary onto Display 2.
4. Verify that the window appears with zero visual tearing and responsive cursor/touch control on the connected Android tablet or iPad.
