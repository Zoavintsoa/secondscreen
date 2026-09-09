/**
 * TelemetryHUD Component
 * Real-time performance HUD with Professional Polish design:
 * High-contrast monospace metric readouts, clean zinc panels, and hardware pipeline breakdowns.
 * Strictly distinguishes MEASURED vs ESTIMATED values.
 */

import React from 'react';
import { useSecondScreen } from '../../context/SecondScreenContext';
import {
  Activity,
  Zap,
  Radio,
  Wifi,
  TrendingUp
} from 'lucide-react';

export const TelemetryHUD: React.FC = () => {
  const {
    telemetry,
    telemetryHistory,
    hostState,
    clientState,
  } = useSecondScreen();
  const value = (metric: number, suffix = '') => telemetry.measured ? `${metric}${suffix}` : 'Not measured';
  const targetStatus = telemetry.measured
    ? telemetry.totalLatencyMs < 16 ? 'TARGET REACHED' : 'TARGET NOT REACHED'
    : 'HARDWARE TEST REQUIRED';

  return (
    <div className="bg-[#0f0f11] border border-[#27272a] rounded-lg p-6 shadow-xl text-[#d4d4d8]">
      {/* HUD Header */}
      <div className="flex items-center justify-between pb-4 border-b border-[#27272a]">
        <div className="flex items-center space-x-3">
          <div className="p-2 bg-blue-600/10 rounded text-blue-400 border border-blue-600/20">
            <Activity className="w-5 h-5" />
          </div>
          <div>
            <h3 className="font-semibold text-white text-sm uppercase tracking-wide">
              Real-Time Telemetry & Hardware Pipeline
            </h3>
            <p className="text-[11px] text-[#71717a] font-mono">
              Hardware measurements appear only after a physical validation run
            </p>
          </div>
        </div>

        <div className="flex items-center space-x-2">
          <span className="px-2.5 py-1 rounded text-[10px] font-mono font-bold bg-emerald-900/30 text-emerald-400 border border-emerald-800/50 flex items-center space-x-1.5 uppercase tracking-wider">
            <span className="w-1.5 h-1.5 rounded-full bg-emerald-400 shadow-[0_0_6px_rgba(16,185,129,0.8)] animate-pulse" />
            <span>{telemetry.measured ? 'MEASURED' : 'NOT MEASURED'}</span>
          </span>
        </div>
      </div>

      {/* Metrics Grid */}
      <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 mt-5">
        {/* Metric 1: FPS */}
        <div className="bg-[#18181b] border border-[#27272a] rounded p-4 text-center">
          <div className="text-sm font-mono font-bold text-white tracking-tight">{value(telemetry.fps)}</div>
          <div className="text-[9px] text-[#71717a] uppercase font-semibold tracking-wider mt-1">Host FPS</div>
          <div className="text-[9px] text-zinc-500 font-mono mt-1">Hardware probe required</div>
        </div>

        {/* Metric 2: End-to-End Latency */}
        <div className="bg-[#18181b] border border-[#27272a] rounded p-4 text-center">
          <div className="text-sm font-mono font-bold text-emerald-400 tracking-tight">{value(telemetry.totalLatencyMs, ' ms')}</div>
          <div className="text-[9px] text-[#71717a] uppercase font-semibold tracking-wider mt-1">E2E Latency</div>
          <div className="text-[9px] text-zinc-500 font-mono mt-1">{targetStatus}</div>
        </div>

        {/* Metric 3: Bitrate */}
        <div className="bg-[#18181b] border border-[#27272a] rounded p-4 text-center">
          <div className="text-sm font-mono font-bold text-blue-400 tracking-tight">{value(telemetry.bitrateMbps)}</div>
          <div className="text-[9px] text-[#71717a] uppercase font-semibold tracking-wider mt-1">Mbps Bitrate</div>
          <div className="text-[9px] text-zinc-500 font-mono mt-1">CBR Buffer</div>
        </div>

        {/* Metric 4: Packet Loss & Jitter */}
        <div className="bg-[#18181b] border border-[#27272a] rounded p-4 text-center">
          <div className="text-sm font-mono font-bold text-white tracking-tight">{value(telemetry.packetLossPercent, '%')}</div>
          <div className="text-[9px] text-[#71717a] uppercase font-semibold tracking-wider mt-1">Packet Loss</div>
          <div className="text-[9px] text-zinc-500 font-mono mt-1">Jitter: {telemetry.measured ? `${telemetry.jitterMs} ms` : 'Not measured'}</div>
        </div>
      </div>

      {/* Latency & Hardware Pipeline Breakdown */}
      <div className="mt-4 bg-[#0a0a0b] border border-[#27272a] rounded p-4">
        <h4 className="text-[10px] font-bold text-[#71717a] uppercase tracking-widest mb-3">
          Hardware Capture & Decoding Pipeline
        </h4>

        <div className="grid grid-cols-1 sm:grid-cols-3 gap-3 text-xs">
          <div className="p-3 bg-[#18181b] rounded border border-[#27272a]">
            <span className="text-[#71717a] block text-[10px] uppercase font-bold tracking-wider">Host Encoder</span>
            <span className="font-semibold text-blue-400 font-mono mt-1 block">{hostState.encoder}</span>
            <span className="text-[10px] text-zinc-500 font-mono">Selected: {hostState.selectedCodec} (unverified)</span>
          </div>

          <div className="p-3 bg-[#18181b] rounded border border-[#27272a]">
            <span className="text-[#71717a] block text-[10px] uppercase font-bold tracking-wider">Client Decoder</span>
            <span className="font-semibold text-emerald-400 font-mono mt-1 block">{clientState.decoder}</span>
            <span className="text-[10px] text-zinc-500 font-mono">Configured decoder (unverified)</span>
          </div>

          <div className="p-3 bg-[#18181b] rounded border border-[#27272a]">
            <span className="text-[#71717a] block text-[10px] uppercase font-bold tracking-wider">Virtual Display ID</span>
            <span className="font-semibold text-white font-mono mt-1 block">Not detected</span>
            <span className="text-[10px] text-zinc-500 font-mono">Requires physical Windows validation</span>
          </div>
        </div>
      </div>

      {/* Latency History Bar Chart */}
      <div className="mt-4 pt-3 border-t border-[#27272a] flex items-center justify-between text-xs text-[#71717a]">
        <span className="flex items-center space-x-1.5 text-[10px] uppercase font-bold tracking-wider">
          <TrendingUp className="w-3.5 h-3.5 text-blue-500" />
          <span>Measured Stream History</span>
        </span>
        <div className="flex items-center space-x-2">
          {telemetryHistory.length === 0 && <span className="text-[10px] font-mono">No hardware samples</span>}
          {telemetryHistory.map((item, index) => (
            <div key={index} className="flex flex-col items-center space-y-1">
              <div
                className="w-3 bg-blue-600 rounded-t transition-all duration-300 shadow-[0_0_6px_rgba(37,99,235,0.4)]"
                style={{ height: `${Math.min(32, Math.max(6, item.latency * 1.4))}px` }}
                title={`${item.time}: ${item.latency}ms, ${item.fps}fps`}
              />
              <span className="text-[8px] font-mono text-[#71717a]">{item.latency}ms</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};
