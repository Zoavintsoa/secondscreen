/**
 * DiagnosticsView Component
 * Platform Diagnostics, 4 Cross-Platform Combinations Matrix,
 * and Strict Feature Status Table with Professional Polish design.
 */

import React from 'react';
import { useSecondScreen } from '../../context/SecondScreenContext';
import { FEATURE_MATRIX, CROSS_PLATFORM_COMBINATIONS } from '../../data/featureMatrix';
import {
  CheckCircle2,
  Play,
  RotateCw,
  Activity,
  Layers,
  Cpu
} from 'lucide-react';
import { FeatureStatus } from '../../types';

export const DiagnosticsView: React.FC = () => {
  const {
    diagnostics,
    runFullDiagnosticCheck,
    isDiagnosticRunning,
  } = useSecondScreen();

  const getStatusBadge = (status: FeatureStatus) => {
    switch (status) {
      case FeatureStatus.IMPLEMENTED:
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-emerald-900/30 text-emerald-400 border border-emerald-800/50 uppercase tracking-wider">
            IMPLEMENTED
          </span>
        );
      case FeatureStatus.IMPLEMENTED_UNTESTED:
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-amber-900/30 text-amber-300 border border-amber-800/50 uppercase tracking-wider">
            IMPLEMENTED / UNTESTED
          </span>
        );
      case FeatureStatus.PROTOTYPE:
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-blue-900/30 text-blue-400 border border-blue-800/50 uppercase tracking-wider">
            PROTOTYPE
          </span>
        );
      case FeatureStatus.SIMULATION:
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-purple-900/30 text-purple-400 border border-purple-800/50 uppercase tracking-wider">
            SIMULATION
          </span>
        );
      case FeatureStatus.ARCHITECTURE_ONLY:
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-zinc-800 text-zinc-300 border border-zinc-700 uppercase tracking-wider">
            ARCHITECTURE_ONLY
          </span>
        );
      case FeatureStatus.NOT_IMPLEMENTED:
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-[#18181b] text-zinc-400 border border-[#27272a] uppercase tracking-wider">
            NOT_IMPLEMENTED
          </span>
        );
      case FeatureStatus.PENDING:
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-blue-950 text-blue-300 border border-blue-800/60 uppercase tracking-wider">
            PENDING
          </span>
        );
      case FeatureStatus.BLOCKED:
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-rose-900/30 text-rose-400 border border-rose-800/50 uppercase tracking-wider">
            BLOCKED
          </span>
        );
      default:
        return null;
    }
  };

  return (
    <div className="flex-1 p-6 bg-[#0a0a0b] text-[#d4d4d8] overflow-y-auto">
      <div className="max-w-7xl mx-auto space-y-6">
        {/* Header & Run Diagnostic Action */}
        <div className="bg-[#0f0f11] border border-[#27272a] rounded-lg p-5 shadow-xl flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div className="flex items-center space-x-3.5">
            <div className="p-2.5 bg-blue-600/10 rounded text-blue-400 border border-blue-600/20">
              <Activity className="w-5 h-5" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h2 className="text-base font-bold text-white uppercase tracking-wide">
                  System Diagnostics & Cross-Platform Matrix
                </h2>
                <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-amber-900/30 text-amber-400 border border-amber-800/50 uppercase tracking-wider">
                  BLOCKED — HARDWARE REQUIRED
                </span>
              </div>
              <p className="text-xs text-[#71717a] mt-0.5 font-mono">
                This browser build cannot validate drivers, GPU capture, encoders, network transport, or mobile decoders.
              </p>
            </div>
          </div>

          <button
            type="button"
            onClick={runFullDiagnosticCheck}
            disabled={isDiagnosticRunning}
            className="px-4 py-2 bg-blue-600 hover:bg-blue-500 disabled:opacity-50 text-white text-xs font-semibold rounded transition-all shadow flex items-center space-x-2 cursor-pointer"
          >
            {isDiagnosticRunning ? (
              <RotateCw className="w-3.5 h-3.5 animate-spin" />
            ) : (
              <Play className="w-3.5 h-3.5" />
            )}
            <span>{isDiagnosticRunning ? 'Checking Validation Prerequisites...' : 'Check Validation Prerequisites'}</span>
          </button>
        </div>

        {/* 4 Cross-Platform Combinations Grid */}
        <div className="bg-[#0f0f11] border border-[#27272a] rounded-lg p-5 shadow-xl space-y-4">
          <div className="flex items-center justify-between pb-3 border-b border-[#27272a]">
            <h3 className="font-semibold text-white text-xs uppercase tracking-wider flex items-center space-x-2">
              <Layers className="w-4 h-4 text-blue-500" />
              <span>4 Cross-Platform Combinations Matrix (Windows/macOS ⇄ Android/iPadOS)</span>
            </h3>
            <span className="text-[10px] text-[#71717a] font-mono">Matrix Target: 4/4 Supported</span>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            {CROSS_PLATFORM_COMBINATIONS.map((combo) => (
              <div
                key={combo.id}
                className="bg-[#18181b] border border-[#27272a] rounded p-4 space-y-3 hover:border-zinc-700 transition-colors"
              >
                <div className="flex items-center justify-between">
                  <span className="font-bold text-white text-sm">{combo.title}</span>
                  {getStatusBadge(combo.overallStatus)}
                </div>

                <div className="text-xs space-y-1.5 font-mono text-[#71717a]">
                  <div className="flex justify-between">
                    <span className="text-zinc-500">Host OS Req:</span>
                    <span className="text-zinc-300 text-right truncate max-w-[240px]">{combo.minHostOS}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-zinc-500">Client OS Req:</span>
                    <span className="text-zinc-300 text-right truncate max-w-[240px]">{combo.minClientOS}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-zinc-500">Transport:</span>
                    <span className="text-blue-400">{combo.transport}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-zinc-500">Target Latency:</span>
                    <span className="text-emerald-400 font-bold">{combo.targetLatency}</span>
                  </div>
                </div>

                <p className="text-[11px] text-[#71717a] pt-2 border-t border-[#27272a] leading-relaxed">
                  {combo.notes}
                </p>
              </div>
            ))}
          </div>
        </div>

        {/* Live System Diagnostics Tests */}
        <div className="bg-[#0f0f11] border border-[#27272a] rounded-lg p-5 shadow-xl space-y-4">
          <h3 className="font-semibold text-white text-xs uppercase tracking-wider flex items-center space-x-2 pb-3 border-b border-[#27272a]">
            <Cpu className="w-4 h-4 text-emerald-500" />
            <span>Hardware & Network Diagnostic Probes</span>
          </h3>

          <div className="space-y-2">
            {diagnostics.map((diag) => (
              <div
                key={diag.id}
                className="bg-[#18181b] border border-[#27272a] rounded p-3.5 flex flex-col sm:flex-row sm:items-center justify-between gap-3 text-xs"
              >
                <div className="space-y-0.5">
                  <div className="font-semibold text-zinc-200 flex items-center space-x-2">
                    <CheckCircle2 className={`w-4 h-4 shrink-0 ${diag.status === 'PASS' ? 'text-emerald-400' : 'text-amber-400'}`} />
                    <span>{diag.name}</span>
                    <span className="text-[9px] bg-[#0a0a0b] text-zinc-400 px-1.5 py-0.2 rounded font-mono border border-[#27272a]">
                      {diag.category}
                    </span>
                  </div>
                  <p className="text-[11px] text-[#71717a] ml-6">{diag.detail}</p>
                </div>

                <div className="sm:text-right shrink-0 font-mono text-[11px] ml-6 sm:ml-0">
                  <div className={diag.isRealMetric ? 'text-emerald-400 font-semibold' : 'text-amber-400 font-semibold'}>{diag.measuredValue}</div>
                  <div className="text-[10px] text-zinc-500">Target: {diag.expectedValue}</div>
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Complete Feature Status Table */}
        <div className="bg-[#0f0f11] border border-[#27272a] rounded-lg p-5 shadow-xl space-y-4">
          <div className="flex items-center justify-between pb-3 border-b border-[#27272a]">
            <h3 className="font-semibold text-white text-xs uppercase tracking-wider flex items-center space-x-2">
              <Layers className="w-4 h-4 text-purple-400" />
              <span>Complete Component Feature Matrix & Strict Statuses</span>
            </h3>
            <span className="text-[10px] text-[#71717a] font-mono">Strict REAL vs SIMULATION flags</span>
          </div>

          <div className="overflow-x-auto">
            <table className="w-full text-left text-xs font-mono">
              <thead className="bg-[#0a0a0b] text-[#71717a] border-b border-[#27272a] uppercase text-[9px] tracking-wider">
                <tr>
                  <th className="p-3">Feature Name</th>
                  <th className="p-3">Platform</th>
                  <th className="p-3">Status</th>
                  <th className="p-3">Technology Stack</th>
                  <th className="p-3">Tested</th>
                  <th className="p-3">Engineering Notes</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-[#27272a] text-zinc-300">
                {FEATURE_MATRIX.map((item) => (
                  <tr key={item.id} className="hover:bg-[#18181b]/50 transition-colors">
                    <td className="p-3 font-semibold text-white">{item.feature}</td>
                    <td className="p-3 text-blue-400">{item.platform}</td>
                    <td className="p-3">{getStatusBadge(item.status)}</td>
                    <td className="p-3 text-[#71717a]">{item.technology}</td>
                    <td className="p-3">
                      {item.tested ? (
                        <span className="text-emerald-400 font-bold">YES</span>
                      ) : (
                        <span className="text-zinc-500">PENDING</span>
                      )}
                    </td>
                    <td className="p-3 text-[11px] text-[#71717a] max-w-xs">{item.notes}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );
};
