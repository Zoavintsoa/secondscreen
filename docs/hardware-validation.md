# SecondScreen - Hardware Validation & DaVinci Resolve Test Protocol

## 1. Technical Truth Rule & Validation Protocol
In accordance with Section 4 and Section 16 of the SecondScreen engineering guidelines:
- **Source Code ≠ Validated Feature**
- **Simulation ≠ Real Functionality**
- **Canvas ≠ Virtual Display**
- **Displayed Metric ≠ Real Measurement**

A feature combination is only considered **PASSED** after physical execution on actual hardware with real measured metrics.

---

## 2. Phase 2 Component Verification Matrix

| Component | Architecture Status | Physical Hardware Status | Validation Requirement |
|---|---|---|---|
| Windows IddCx Driver | **READY** | `UNTESTED` | Windows 10/11 + WDK Installation |
| Windows DXGI Capture | **READY** | `UNTESTED` | Direct3D 11 GPU Desktop Duplication |
| Hardware Encoder (NVENC/QSV/AMF) | **READY** | `UNTESTED` | Physical NVIDIA/Intel/AMD GPU |
| Binary Network Protocol v1 | **READY** | `UNTESTED` | WinSock2 LAN Socket Connection |
| Android MediaCodec Client | **READY** | `UNTESTED` | Physical Android Tablet (API 26+) |
| Windows → Android Pipeline | **READY** | `PENDING` | Physical End-to-End Test Run |
| DaVinci Resolve Integration | **READY** | `BLOCKED` | Windows Host + DaVinci Resolve Studio |

---

## 3. DaVinci Resolve Video Scopes Test Procedure

### Objective:
Verify that Display 2 functions as a full-fledged secondary monitor with real-time video scopes driven by an actual 4K timeline in DaVinci Resolve Studio.

### Step-by-Step Execution:
1. **Host Driver Activation**:
   - Install the SecondScreen IddCx driver on the Windows host.
   - Confirm Display 2 is recognized in Windows Settings and set to **Extend Desktop**.
2. **Start Network Host**:
   - Launch the SecondScreen Host Server on port 9876.
3. **Connect Android Client**:
   - Open SecondScreen on the Android tablet.
   - Enter the 6-digit PIN to establish the low-latency MediaCodec stream.
   - Verify fullscreen mode is active.
4. **DaVinci Resolve Setup**:
   - Launch DaVinci Resolve on the Windows Host.
   - Open a project with a 4K ProRes or RAW video clip.
   - In DaVinci Resolve, select **Workspace → Video Scopes → Dual Screen / Scopes Window**.
   - Move the Video Scopes window onto **Display 2**.
   - Enable:
     - **RGB Parade**
     - **Waveform**
     - **Vectorscope**
     - **Histogram**
5. **Real-Time Playback Verification**:
   - Hit Play on the 4K timeline.
   - Observe the live scopes rendered on the Android tablet.
   - Perform color wheel adjustments (Lift, Gamma, Gain) and confirm immediate scope deflection on the tablet.
6. **10-Minute Stability Run**:
   - Run continuous playback for 10 minutes.
   - Record measured FPS, RTT latency, packet loss, and frame drops.
   - Confirm zero unhandled reconnects or driver crashes.
