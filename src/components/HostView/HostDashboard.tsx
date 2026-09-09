/**
 * HostDashboard Component
 * Complete Host Control Center for Windows and macOS with Professional Polish theme.
 * Manages Virtual Display 2, GPU Encoders, Codecs, Quality Presets,
 * Display Topology (Extend/Mirror), and Network Server Socket.
 */

import React, { useState } from 'react';
import { useSecondScreen } from '../../context/SecondScreenContext';
import { PairingModal } from './PairingModal';
import { TelemetryHUD } from './TelemetryHUD';
import {
  Tv,
  Cpu,
  Power,
  Server,
  Lock,
  Layers,
  Sparkles
} from 'lucide-react';
import { Platform } from '../../types';
import {
  DisplayMode,
  VideoCodec,
  EncoderType,
  QualityPreset
} from '../../types/protocol';

export const HostDashboard: React.FC = () => {
  const {
    hostState,
    setHostOS,
    toggleVirtualDisplay,
    setHostDisplayMode,
    setHostEncoder,
    setHostCodec,
    setHostQualityPreset,
  } = useSecondScreen();

  const [isPairingModalOpen, setIsPairingModalOpen] = useState(false);

  return (
    <div className="flex-1 p-6 bg-[#0a0a0b] text-[#d4d4d8] overflow-y-auto font-sans">
      <div className="max-w-6xl mx-auto space-y-6">
        {/* Top Host Header & Platform Switcher */}
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4 bg-[#121214] border border-[#27272a] p-5 rounded-lg shadow-xl">
          <div className="flex items-center space-x-3.5">
            <div className="p-3 bg-blue-600/10 rounded-lg text-blue-400 border border-blue-600/20 shadow-[0_0_15px_rgba(37,99,235,0.15)]">
              <Server className="w-6 h-6" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h2 className="text-lg font-semibold tracking-tight uppercase text-white">
                  SecondScreen Host Engine
                </h2>
                <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-amber-900/30 text-amber-400 border border-amber-800/50 uppercase tracking-wider">
                  NOT VALIDATED (Port {hostState.port})
                </span>
              </div>
              <p className="text-xs text-[#71717a] font-mono mt-0.5">
                Host IP: {hostState.ipAddress} • System: {hostState.hostName}
              </p>
            </div>
          </div>

          {/* OS Switcher & Pairing Modal Button */}
          <div className="flex items-center space-x-3">
            <div className="flex bg-[#0a0a0b] p-1 rounded-lg border border-[#27272a]">
              <button
                type="button"
                onClick={() => setHostOS(Platform.WINDOWS)}
                className={`px-3 py-1.5 text-xs font-semibold rounded transition-all uppercase tracking-wider ${
                  hostState.os === Platform.WINDOWS
                    ? 'bg-blue-600 text-white shadow-sm'
                    : 'text-[#71717a] hover:text-[#d4d4d8]'
                }`}
              >
                Windows (IddCx)
              </button>
              <button
                type="button"
                onClick={() => setHostOS(Platform.MACOS)}
                className={`px-3 py-1.5 text-xs font-semibold rounded transition-all uppercase tracking-wider ${
                  hostState.os === Platform.MACOS
                    ? 'bg-[#27272a] text-white shadow-sm'
                    : 'text-[#71717a] hover:text-[#d4d4d8]'
                }`}
              >
                macOS (CGVirtualDisplay)
              </button>
            </div>

            <button
              type="button"
              onClick={() => setIsPairingModalOpen(true)}
              className="px-4 py-2 bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold rounded transition-all shadow-[0_0_15px_rgba(37,99,235,0.3)] flex items-center space-x-2 uppercase tracking-wider"
            >
              <Lock className="w-4 h-4" />
              <span>PIN ({hostState.pairingPin})</span>
            </button>
          </div>
        </div>

        {/* Display Topology & Virtual Display 2 Configuration */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {/* Virtual Display 2 Controller */}
          <div className="lg:col-span-2 bg-[#0f0f11] border border-[#27272a] rounded-lg p-6 shadow-xl space-y-5">
            <div className="flex items-center justify-between pb-3 border-b border-[#27272a]">
              <div className="flex items-center space-x-2">
                <Tv className="w-5 h-5 text-blue-500" />
                <h3 className="font-semibold text-white text-sm uppercase tracking-wide">
                  Virtual Display 2 Controller
                </h3>
              </div>
              <button
                type="button"
                onClick={toggleVirtualDisplay}
                className={`px-3 py-1.5 rounded text-xs font-mono font-semibold flex items-center space-x-1.5 transition-all ${
                  hostState.virtualDisplayActive
                    ? 'bg-emerald-900/30 text-emerald-400 border border-emerald-800/50 shadow-[0_0_10px_rgba(16,185,129,0.2)]'
                    : 'bg-[#18181b] text-[#71717a] border border-[#27272a]'
                }`}
              >
                <Power className="w-3.5 h-3.5" />
                <span>{hostState.virtualDisplayActive ? 'DISPLAY 2 ATTACHED' : 'DISPLAY 2 DISABLED'}</span>
              </button>
            </div>

            {/* Display Arrangement Diagram */}
            <div className="bg-[#050505] rounded-lg p-6 border border-[#27272a] flex flex-col sm:flex-row items-center justify-center gap-6">
              {/* Display 1: Primary Monitor */}
              <div className="w-52 h-34 bg-[#121214] border-2 border-[#27272a] rounded p-3 flex flex-col justify-between relative shadow-inner">
                <div className="flex items-center justify-between text-[11px] text-[#71717a]">
                  <span className="font-bold text-white uppercase tracking-wider">Display 1</span>
                  <span className="text-[10px] bg-[#18181b] px-1.5 py-0.5 rounded text-zinc-300 font-mono">
                    Primary
                  </span>
                </div>
                <div className="text-center font-mono text-xs text-[#d4d4d8]">
                  3840 × 2160 (4K)
                  <span className="block text-[10px] text-[#71717a] mt-0.5">LG UltraFine 60Hz</span>
                </div>
                <div className="text-[10px] text-[#71717a] text-right font-mono">sRGB / Rec.709</div>
              </div>

              {/* Display 2: SecondScreen Virtual Monitor */}
              <div
                className={`w-52 h-34 rounded p-3 flex flex-col justify-between relative transition-all ${
                  hostState.virtualDisplayActive
                    ? 'bg-[#121214] border-2 border-blue-600 shadow-[0_0_20px_rgba(37,99,235,0.2)]'
                    : 'bg-[#0a0a0b] border-2 border-dashed border-[#27272a] opacity-40'
                }`}
              >
                <div className="flex items-center justify-between text-[11px]">
                  <span className="font-bold text-blue-400 uppercase tracking-wider">Display 2</span>
                  <span className="text-[10px] bg-blue-600/20 text-blue-300 border border-blue-500/30 px-1.5 py-0.5 rounded font-mono uppercase">
                    Target configuration
                  </span>
                </div>
                <div className="text-center font-mono text-xs text-white">
                  Resolution not detected
                  <span className="block text-[10px] text-blue-400 mt-0.5">Target: validate on Windows + Android</span>
                </div>
                <div className="flex items-center justify-between text-[10px] text-[#71717a] font-mono">
                  <span>Not measured</span>
                  <span className="text-amber-400 font-bold">TEST REQUIRED</span>
                </div>
              </div>
            </div>

            {/* Display Modes Selector */}
            <div className="space-y-2">
              <h2 className="text-[10px] text-[#71717a] font-bold uppercase tracking-widest">
                Display Extension Mode
              </h2>
              <div className="grid grid-cols-1 sm:grid-cols-3 gap-2">
                {[
                  { mode: DisplayMode.EXTEND, label: 'Extend Desktop', desc: 'Separate Display 2 for DaVinci Scopes' },
                  { mode: DisplayMode.DUPLICATE, label: 'Duplicate / Mirror', desc: 'Mirror Display 1 to Tablet' },
                  { mode: DisplayMode.SECOND_SCREEN_ONLY, label: 'Second Screen Only', desc: 'Main Screen Off, Tablet Only' },
                ].map((item) => (
                  <button
                    key={item.mode}
                    type="button"
                    onClick={() => setHostDisplayMode(item.mode)}
                    className={`p-3 rounded border text-left transition-all ${
                      hostState.displays.find((d) => d.isVirtual)?.mode === item.mode
                        ? 'bg-blue-600/10 border-blue-600/60 text-white shadow-sm'
                        : 'bg-[#18181b] border-[#27272a] text-[#71717a] hover:border-[#3f3f46] hover:text-[#d4d4d8]'
                    }`}
                  >
                    <div className="font-semibold text-xs text-zinc-200">{item.label}</div>
                    <div className="text-[10px] text-[#71717a] mt-0.5">{item.desc}</div>
                  </button>
                ))}
              </div>
            </div>
          </div>

          {/* GPU Hardware Encoder & Stream Profiles */}
          <div className="bg-[#0f0f11] border border-[#27272a] rounded-lg p-6 shadow-xl space-y-5">
            <div className="flex items-center space-x-2 pb-3 border-b border-[#27272a]">
              <Cpu className="w-5 h-5 text-blue-500" />
              <h3 className="font-semibold text-white text-sm uppercase tracking-wide">
                Encoder & Stream Profile
              </h3>
            </div>

            {/* Hardware Encoder */}
            <div className="space-y-1.5">
              <label className="text-[10px] text-[#71717a] font-bold uppercase tracking-widest block">
                Hardware Video Encoder
              </label>
              <select
                value={hostState.encoder}
                onChange={(e) => setHostEncoder(e.target.value as EncoderType)}
                aria-label="Hardware Video Encoder"
                className="w-full bg-[#18181b] border border-[#27272a] text-[#d4d4d8] text-xs rounded p-2.5 font-mono focus:outline-none focus:border-blue-500"
              >
                {hostState.os === Platform.WINDOWS ? (
                  <>
                    <option value={EncoderType.NVENC}>NVIDIA NVENC (Low Latency CBR)</option>
                    <option value={EncoderType.AMF}>AMD AMF (Advanced Media Framework)</option>
                    <option value={EncoderType.QUICKSYNC}>Intel Quick Sync Video (QSV)</option>
                    <option value={EncoderType.SOFTWARE}>Software CPU (libx264 Fallback)</option>
                  </>
                ) : (
                  <>
                    <option value={EncoderType.VIDEOTOOLBOX}>Apple VideoToolbox (M-Series)</option>
                    <option value={EncoderType.SOFTWARE}>Software CPU (libx264 Fallback)</option>
                  </>
                )}
              </select>
            </div>

            {/* Video Codec */}
            <div className="space-y-1.5">
              <label className="text-[10px] text-[#71717a] font-bold uppercase tracking-widest block">
                Compression Codec
              </label>
              <div className="grid grid-cols-3 gap-2">
                {[VideoCodec.H264, VideoCodec.HEVC, VideoCodec.AV1].map((codec) => (
                  <button
                    key={codec}
                    type="button"
                    onClick={() => setHostCodec(codec)}
                    className={`py-2 px-2 rounded border text-center text-xs font-mono font-medium transition-all ${
                      hostState.selectedCodec === codec
                        ? 'bg-blue-600/20 border-blue-500 text-blue-300'
                        : 'bg-[#18181b] border-[#27272a] text-[#71717a] hover:border-[#3f3f46]'
                    }`}
                  >
                    {codec.split(' ')[0]}
                  </button>
                ))}
              </div>
            </div>

            {/* Quality Preset */}
            <div className="space-y-1.5">
              <label className="text-[10px] text-[#71717a] font-bold uppercase tracking-widest block">
                Streaming Preset
              </label>
              <div className="space-y-2">
                {[
                  { preset: QualityPreset.LOW_LATENCY, label: 'Low Latency', desc: '1080p @ 60fps (15 Mbps, <10ms)' },
                  { preset: QualityPreset.BALANCED, label: 'Balanced (Standard)', desc: '1440p @ 60fps (25 Mbps)' },
                  { preset: QualityPreset.QUALITY, label: 'Mastering Quality', desc: '1600p @ 60fps (40 Mbps, DCI-P3)' },
                ].map((item) => (
                  <button
                    key={item.preset}
                    type="button"
                    onClick={() => setHostQualityPreset(item.preset)}
                    className={`w-full p-2.5 rounded border text-left transition-all ${
                      hostState.qualityPreset === item.preset
                        ? 'bg-blue-600/10 border-blue-600/60 text-white'
                        : 'bg-[#18181b] border-[#27272a] text-[#71717a] hover:border-[#3f3f46]'
                    }`}
                  >
                    <div className="font-semibold text-xs text-zinc-200">{item.label}</div>
                    <div className="text-[10px] text-[#71717a]">{item.desc}</div>
                  </button>
                ))}
              </div>
            </div>
          </div>
        </div>

        {/* Real-time Telemetry Monitor HUD */}
        <TelemetryHUD />

        {/* Pairing Modal */}
        <PairingModal
          isOpen={isPairingModalOpen}
          onClose={() => setIsPairingModalOpen(false)}
        />
      </div>
    </div>
  );
};
