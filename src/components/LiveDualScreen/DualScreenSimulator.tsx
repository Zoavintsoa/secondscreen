/**
 * DualScreenSimulator Component
 * Live interactive Dual Screen simulation executing the DaVinci Resolve professional test
 * with the Professional Polish dark theme.
 *
 * Demonstrates:
 * Display 1 (PC/Mac): DaVinci Resolve primary grading UI with live Lift/Gamma/Gain wheels & sliders.
 * Display 2 (Tablet/iPad): SecondScreen Virtual Display streaming real-time RGB Parade, Waveform,
 * Vectorscope, and Histogram responding directly to color grade tweaks.
 */

import React from 'react';
import { useSecondScreen } from '../../context/SecondScreenContext';
import { DisplaySurface } from '../ClientOnlyView/DisplaySurface';
import {
  Monitor,
  Tablet,
  Smartphone,
  Sliders,
  Sparkles,
  Activity,
  RefreshCw,
} from 'lucide-react';
import { Platform } from '../../types';

export const DualScreenSimulator: React.FC = () => {
  const {
    hostState,
    clientState,
    setClientPlatform,
    daVinciState,
    updateColorGrade,
    resetColorGrade,
    toggleScopesOnDisplay2,
    telemetry,
  } = useSecondScreen();

  return (
    <div className="flex-1 p-6 bg-[#0a0a0b] text-[#d4d4d8] overflow-y-auto">
      <div className="max-w-7xl mx-auto space-y-6">
        {/* Workspace Banner & Test Mode Description */}
        <div className="bg-[#0f0f11] border border-[#27272a] rounded-lg p-5 shadow-xl flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div className="flex items-center space-x-3.5">
            <div className="p-2.5 bg-blue-600/10 rounded text-blue-400 border border-blue-600/20">
              <Sparkles className="w-5 h-5" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h2 className="text-base font-bold text-white uppercase tracking-wide">
                  Live Dual-Screen & DaVinci Resolve Scopes Testbed
                </h2>
                <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-blue-900/30 text-blue-400 border border-blue-800/50 uppercase tracking-wider">
                  SECTION 2 & 21 TEST
                </span>
              </div>
              <p className="text-xs text-[#71717a] mt-0.5 font-mono">
                Host OS recognizes Display 2 as a physical virtual monitor. DaVinci Resolve docks Video Scopes directly onto Display 2.
              </p>
            </div>
          </div>

          <div className="flex items-center space-x-3">
            <button
              type="button"
              onClick={toggleScopesOnDisplay2}
              className={`px-3.5 py-2 rounded text-xs font-semibold flex items-center space-x-1.5 transition-all ${
                daVinciState.scopesOnDisplay2
                  ? 'bg-emerald-900/30 text-emerald-400 border border-emerald-800/50 shadow'
                  : 'bg-[#18181b] text-zinc-400 border border-[#27272a]'
              }`}
            >
              <Activity className="w-3.5 h-3.5" />
              <span>{daVinciState.scopesOnDisplay2 ? 'Scopes Docked on Display 2 (Tablet)' : 'Scopes on Display 1 (Main PC)'}</span>
            </button>

            <button
              type="button"
              onClick={resetColorGrade}
              className="p-2 bg-[#18181b] hover:bg-[#27272a] text-zinc-300 rounded border border-[#27272a] transition-colors"
              title="Reset Color Grading"
            >
              <RefreshCw className="w-4 h-4" />
            </button>
          </div>
        </div>

        {/* Dual Screen Split Presentation */}
        <div className="grid grid-cols-1 xl:grid-cols-12 gap-6">
          {/* LEFT: Host Display 1 - DaVinci Resolve Studio UI */}
          <div className="xl:col-span-7 bg-[#0f0f11] border border-[#27272a] rounded-lg p-5 shadow-xl flex flex-col space-y-4">
            <div className="flex items-center justify-between pb-3 border-b border-[#27272a]">
              <div className="flex items-center space-x-2">
                <Monitor className="w-4 h-4 text-blue-500" />
                <span className="font-semibold text-white text-xs uppercase tracking-wider">
                  Display 1 — {hostState.hostName} ({hostState.os})
                </span>
                <span className="text-[10px] bg-[#18181b] px-2 py-0.5 rounded text-zinc-400 font-mono border border-[#27272a]">
                  Primary 3840×2160
                </span>
              </div>
              <span className="text-[11px] text-[#71717a] font-mono">DaVinci Resolve Studio</span>
            </div>

            {/* Cinema Video Preview Canvas with Dynamic Color Grading */}
            <div className="relative w-full aspect-video bg-black rounded overflow-hidden border border-[#27272a] flex items-center justify-center shadow-inner">
              {/* Stylized Film Scene */}
              <div
                className="w-full h-full relative flex items-center justify-center transition-all duration-100"
                style={{
                  filter: `
                    contrast(${daVinciState.contrast}%)
                    saturate(${daVinciState.saturation}%)
                    brightness(${100 + daVinciState.gain.y * 20}%)
                    sepia(${Math.max(0, daVinciState.colorTemperature) * 0.8}%)
                    hue-rotate(${daVinciState.lift.r * 15}deg)
                  `,
                }}
              >
                {/* Background cinematic atmospheric gradient */}
                <div className="absolute inset-0 bg-gradient-to-tr from-zinc-950 via-slate-900 to-amber-950 opacity-90" />
                <div className="absolute inset-0 bg-[radial-gradient(circle_at_center,_var(--tw-gradient-stops))] from-blue-500/10 via-transparent to-black/80" />

                {/* Center visual object representing 4K ProRes Film Sample */}
                <div className="z-10 text-center space-y-2 p-6">
                  <div className="w-20 h-20 mx-auto rounded-xl bg-gradient-to-tr from-blue-400 via-rose-500 to-amber-400 blur-sm opacity-80 animate-pulse" />
                  <div className="font-mono text-xs text-zinc-200 tracking-widest uppercase font-bold">
                    ARRI Alexa Mini LF • 4.5K ProRes 4444 XQ
                  </div>
                  <div className="text-[10px] text-[#71717a] font-mono">
                    Timecode: 01:24:18:12 • Rec.709 Gamma 2.4
                  </div>
                </div>

                {/* Grid Overlay */}
                <div className="absolute inset-0 border border-white/5 grid grid-cols-3 grid-rows-3 pointer-events-none" />
              </div>

              {/* Resolution & Timeline Pill */}
              <div className="absolute bottom-3 left-3 bg-[#0a0a0b]/80 backdrop-blur-md px-2.5 py-1 rounded text-[10px] font-mono text-zinc-300 border border-[#27272a]">
                100% 4K DCI • Timeline Playback: 24.000 fps
              </div>
            </div>

            {/* Interactive DaVinci Primary Color Grading Controls */}
            <div className="bg-[#0a0a0b] rounded p-4 border border-[#27272a] space-y-4">
              <div className="flex items-center justify-between text-xs pb-2 border-b border-[#27272a]">
                <span className="font-bold text-white uppercase text-[10px] tracking-widest flex items-center space-x-1.5">
                  <Sliders className="w-3.5 h-3.5 text-blue-500" />
                  <span>Color Wheels & Primary Adjustments</span>
                </span>
                <span className="text-[10px] text-[#71717a] font-mono">
                  Move sliders to see real-time Scopes update on Display 2
                </span>
              </div>

              <div className="grid grid-cols-1 sm:grid-cols-3 gap-3 text-xs font-mono">
                {/* Lift (Shadows) */}
                <div className="bg-[#18181b] p-3 rounded border border-[#27272a] space-y-2">
                  <div className="flex justify-between text-zinc-300 font-semibold text-[11px]">
                    <span>LIFT (Shadows)</span>
                    <span className="text-blue-400">{daVinciState.lift.r.toFixed(2)}</span>
                  </div>
                  <input
                    type="range"
                    min={-1}
                    max={1}
                    step={0.05}
                    value={daVinciState.lift.r}
                    onChange={(e) =>
                      updateColorGrade({
                        lift: { ...daVinciState.lift, r: parseFloat(e.target.value), g: parseFloat(e.target.value) * 0.8, b: parseFloat(e.target.value) * 0.6 },
                      })
                    }
                    className="w-full accent-blue-500 cursor-pointer"
                  />
                  <div className="text-[9px] text-[#71717a]">Master Offset: Red/Blue Blacks</div>
                </div>

                {/* Gamma (Midtones) */}
                <div className="bg-[#18181b] p-3 rounded border border-[#27272a] space-y-2">
                  <div className="flex justify-between text-zinc-300 font-semibold text-[11px]">
                    <span>GAMMA (Midtones)</span>
                    <span className="text-emerald-400">{daVinciState.gamma.r.toFixed(2)}</span>
                  </div>
                  <input
                    type="range"
                    min={-1}
                    max={1}
                    step={0.05}
                    value={daVinciState.gamma.r}
                    onChange={(e) =>
                      updateColorGrade({
                        gamma: { ...daVinciState.gamma, r: parseFloat(e.target.value), g: parseFloat(e.target.value), b: parseFloat(e.target.value) },
                      })
                    }
                    className="w-full accent-emerald-500 cursor-pointer"
                  />
                  <div className="text-[9px] text-[#71717a]">Mid-Curve Exposure</div>
                </div>

                {/* Gain (Highlights) */}
                <div className="bg-[#18181b] p-3 rounded border border-[#27272a] space-y-2">
                  <div className="flex justify-between text-zinc-300 font-semibold text-[11px]">
                    <span>GAIN (Highlights)</span>
                    <span className="text-amber-400">{daVinciState.gain.y.toFixed(2)}</span>
                  </div>
                  <input
                    type="range"
                    min={-1}
                    max={1}
                    step={0.05}
                    value={daVinciState.gain.y}
                    onChange={(e) =>
                      updateColorGrade({
                        gain: { r: parseFloat(e.target.value) * 0.9, g: parseFloat(e.target.value) * 0.7, b: parseFloat(e.target.value) * 0.5, y: parseFloat(e.target.value) },
                      })
                    }
                    className="w-full accent-amber-500 cursor-pointer"
                  />
                  <div className="text-[9px] text-[#71717a]">White Point Threshold</div>
                </div>
              </div>

              {/* Saturation, Contrast, Color Temp */}
              <div className="grid grid-cols-1 sm:grid-cols-3 gap-3 text-xs font-mono pt-1">
                <div className="bg-[#18181b] p-2.5 rounded border border-[#27272a]">
                  <div className="flex justify-between text-[#71717a] text-[10px] mb-1">
                    <span className="uppercase font-bold">Saturation</span>
                    <span className="text-white">{daVinciState.saturation}%</span>
                  </div>
                  <input
                    type="range"
                    min={0}
                    max={100}
                    value={daVinciState.saturation}
                    onChange={(e) => updateColorGrade({ saturation: parseInt(e.target.value) })}
                    className="w-full accent-blue-500 cursor-pointer"
                  />
                </div>

                <div className="bg-[#18181b] p-2.5 rounded border border-[#27272a]">
                  <div className="flex justify-between text-[#71717a] text-[10px] mb-1">
                    <span className="uppercase font-bold">Contrast</span>
                    <span className="text-white">{daVinciState.contrast}%</span>
                  </div>
                  <input
                    type="range"
                    min={0}
                    max={100}
                    value={daVinciState.contrast}
                    onChange={(e) => updateColorGrade({ contrast: parseInt(e.target.value) })}
                    className="w-full accent-emerald-500 cursor-pointer"
                  />
                </div>

                <div className="bg-[#18181b] p-2.5 rounded border border-[#27272a]">
                  <div className="flex justify-between text-[#71717a] text-[10px] mb-1">
                    <span className="uppercase font-bold">Color Temp</span>
                    <span className="text-white">{daVinciState.colorTemperature > 0 ? `+${daVinciState.colorTemperature}` : daVinciState.colorTemperature}</span>
                  </div>
                  <input
                    type="range"
                    min={-50}
                    max={50}
                    value={daVinciState.colorTemperature}
                    onChange={(e) => updateColorGrade({ colorTemperature: parseInt(e.target.value) })}
                    className="w-full accent-amber-500 cursor-pointer"
                  />
                </div>
              </div>
            </div>
          </div>

          {/* RIGHT: SecondScreen Display 2 - Streamed to iPad / Android Tablet */}
          <div className="xl:col-span-5 bg-[#0f0f11] border border-[#27272a] rounded-lg p-5 shadow-xl flex flex-col space-y-4">
            <div className="flex items-center justify-between pb-3 border-b border-[#27272a]">
              <div className="flex items-center space-x-2">
                {clientState.platform === Platform.IOS_IPADOS ? (
                  <Tablet className="w-4 h-4 text-blue-500" />
                ) : (
                  <Smartphone className="w-4 h-4 text-emerald-500" />
                )}
                <span className="font-semibold text-white text-xs uppercase tracking-wider">
                  Display 2 — {clientState.clientName}
                </span>
              </div>

              {/* Tablet Platform Switcher */}
              <div className="flex bg-[#0a0a0b] p-0.5 rounded border border-[#27272a] text-[10px] font-mono">
                <button
                  type="button"
                  onClick={() => setClientPlatform(Platform.IOS_IPADOS)}
                  className={`px-2.5 py-0.5 rounded font-bold uppercase ${
                    clientState.platform === Platform.IOS_IPADOS ? 'bg-blue-600 text-white' : 'text-zinc-400 hover:text-white'
                  }`}
                >
                  iPad Pro
                </button>
                <button
                  type="button"
                  onClick={() => setClientPlatform(Platform.ANDROID)}
                  className={`px-2.5 py-0.5 rounded font-bold uppercase ${
                    clientState.platform === Platform.ANDROID ? 'bg-emerald-600 text-white' : 'text-zinc-400 hover:text-white'
                  }`}
                >
                  Galaxy Tab S9
                </button>
              </div>
            </div>

            {/* DisplaySurface Live Scopes Stream */}
            <div className="w-full aspect-[4/3] bg-black rounded overflow-hidden border border-[#27272a] shadow-2xl">
              {daVinciState.scopesOnDisplay2 ? (
                <DisplaySurface interactive={true} />
              ) : (
                <div className="w-full h-full flex flex-col items-center justify-center p-6 text-center text-zinc-500 space-y-3">
                  <Activity className="w-8 h-8 opacity-30 text-zinc-400" />
                  <p className="text-xs">Scopes currently docked on Display 1 (Main PC)</p>
                  <button
                    type="button"
                    onClick={toggleScopesOnDisplay2}
                    className="px-3 py-1.5 bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold rounded"
                  >
                    Dock Scopes to Display 2 (Tablet)
                  </button>
                </div>
              )}
            </div>

            {/* Live Pipeline Flow Verification */}
            <div className="bg-[#0a0a0b] rounded p-4 border border-[#27272a] space-y-2 text-xs font-mono">
              <div className="flex items-center justify-between text-zinc-300 font-semibold border-b border-[#27272a] pb-2">
                <span className="text-[10px] uppercase font-bold text-[#71717a]">Hardware Pipeline Flow</span>
                <span className="text-emerald-400 font-bold text-[10px]">ACTIVE (60.0 FPS)</span>
              </div>

              <div className="flex items-center justify-between text-[11px] text-[#71717a]">
                <span>1. Virtual Display:</span>
                <strong className="text-zinc-200">{hostState.os === 'WINDOWS' ? 'IddCx Driver 1.4' : 'CGVirtualDisplay'}</strong>
              </div>
              <div className="flex items-center justify-between text-[11px] text-[#71717a]">
                <span>2. GPU Capture:</span>
                <strong className="text-blue-400">{hostState.os === 'WINDOWS' ? 'DXGI 1.2 Desktop Duplication' : 'ScreenCaptureKit'}</strong>
              </div>
              <div className="flex items-center justify-between text-[11px] text-[#71717a]">
                <span>3. Hardware Encoder:</span>
                <strong className="text-zinc-200">{hostState.encoder} (CBR 24.5 Mbps)</strong>
              </div>
              <div className="flex items-center justify-between text-[11px] text-[#71717a]">
                <span>4. Network Transport:</span>
                <strong className="text-emerald-400">UDP/RTP (RTT: {telemetry.rttMs} ms)</strong>
              </div>
              <div className="flex items-center justify-between text-[11px] text-[#71717a]">
                <span>5. Client Decoder:</span>
                <strong className="text-zinc-200">{clientState.decoder}</strong>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

