/**
 * HeaderNav Component
 * Main Navigation bar for SecondScreen application with Professional Polish design theme:
 * Deep zinc/carbon background, crisp blue & emerald accents, and technical micro-typography.
 */

import React, { useState } from 'react';
import { useSecondScreen } from '../context/SecondScreenContext';
import { PairingModal } from './HostView/PairingModal';
import {
  Monitor,
  Tablet,
  Activity,
  Code2,
  Video,
  Lock,
  Sparkles
} from 'lucide-react';
import { Platform } from '../types';

export const HeaderNav: React.FC = () => {
  const {
    activeTab,
    setActiveTab,
    hostState,
    clientState,
    telemetry,
    triggerRealScreenCapture,
    stopRealScreenCapture,
  } = useSecondScreen();

  const [isPairingOpen, setIsPairingOpen] = useState(false);

  const tabs = [
    {
      id: 'DUAL_SCREEN_LAB',
      label: 'Dual-Screen & DaVinci',
      icon: Sparkles,
      tag: 'Scopes Lab',
    },
    {
      id: 'HOST_VIEW',
      label: 'Host Dashboard',
      icon: Monitor,
      tag: hostState.os === Platform.WINDOWS ? 'Win 11' : 'macOS',
    },
    {
      id: 'CLIENT_VIEW',
      label: 'Tablet Client View',
      icon: Tablet,
      tag: clientState.platform === Platform.IOS_IPADOS ? 'iPadOS' : 'Android',
    },
    {
      id: 'DIAGNOSTICS',
      label: 'Diagnostics & Matrix',
      icon: Activity,
      tag: '4/4 Combos',
    },
    {
      id: 'ARCHITECTURE',
      label: 'Native Drivers & Code',
      icon: Code2,
      tag: 'C++/Swift/Kotlin',
    },
  ] as const;

  return (
    <header className="h-16 bg-[#121214] border-b border-[#27272a] px-4 md:px-6 flex items-center justify-between shrink-0 select-none text-[#d4d4d8]">
      {/* Brand Logo & Name */}
      <div className="flex items-center space-x-3">
        <div className="w-8 h-8 bg-blue-600 rounded flex items-center justify-center font-bold text-white italic shadow-[0_0_12px_rgba(37,99,235,0.4)] tracking-tighter text-sm">
          S2
        </div>
        <div>
          <div className="flex items-center space-x-2">
            <h1 className="text-base font-semibold tracking-tight uppercase text-white">
              SecondScreen <span className="text-blue-500 font-extrabold">Pro</span>
            </h1>
            <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-blue-500/10 text-blue-400 border border-blue-500/20 uppercase tracking-widest hidden sm:inline-block">
              V1.0.4
            </span>
          </div>
          <p className="text-[10px] text-[#71717a] font-mono tracking-wider hidden sm:block">
            VIRTUAL DISPLAY WIRE PROTOCOL
          </p>
        </div>
      </div>

      {/* Main Tab Navigation */}
      <nav className="hidden lg:flex items-center bg-[#0a0a0b] p-1 rounded-lg border border-[#27272a]">
        {tabs.map((tab) => {
          const Icon = tab.icon;
          const isActive = activeTab === tab.id;
          return (
            <button
              key={tab.id}
              type="button"
              onClick={() => setActiveTab(tab.id as typeof activeTab)}
              className={`flex items-center space-x-2 px-3 py-1.5 rounded text-xs uppercase tracking-wider font-semibold transition-all ${
                isActive
                  ? 'bg-[#27272a] text-white border border-[#3f3f46] shadow-sm'
                  : 'text-[#71717a] hover:text-[#d4d4d8] hover:bg-[#18181b]'
              }`}
            >
              <Icon className={`w-3.5 h-3.5 ${isActive ? 'text-blue-400' : 'text-[#71717a]'}`} />
              <span>{tab.label}</span>
              {tab.tag && (
                <span className={`text-[9px] px-1.5 py-0.2 rounded font-mono ${
                  isActive ? 'bg-blue-600/20 text-blue-300 border border-blue-500/30' : 'bg-[#18181b] text-[#71717a]'
                }`}>
                  {tab.tag}
                </span>
              )}
            </button>
          );
        })}
      </nav>

      {/* Right System Info & Actions */}
      <div className="flex items-center space-x-3 text-xs">
        {/* Host/Client Status Indicator */}
        <div className="hidden xl:flex items-center space-x-4 uppercase tracking-widest text-[11px]">
          <div className="flex items-center space-x-2">
            <span className="w-2 h-2 rounded-full bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.6)] animate-pulse"></span>
            <span className="text-zinc-300 font-medium">Host: {hostState.os === Platform.WINDOWS ? 'Windows 11' : 'macOS'}</span>
          </div>
          <div className="flex items-center space-x-2 text-[#71717a]">
            <span className="w-2 h-2 rounded-full bg-blue-500"></span>
            <span>Client: {clientState.platform === Platform.IOS_IPADOS ? 'iPad Pro' : 'Android'}</span>
          </div>
        </div>

        {/* Real Capture Action */}
        <button
          type="button"
          onClick={clientState.activeSource === 'REAL_NETWORK' ? stopRealScreenCapture : triggerRealScreenCapture}
          className={`px-3 py-1 rounded text-xs font-mono font-medium flex items-center space-x-1.5 transition-all border ${
            clientState.activeSource === 'REAL_NETWORK'
              ? 'bg-emerald-900/30 text-emerald-400 border-emerald-800/60 shadow-[0_0_10px_rgba(16,185,129,0.2)]'
              : 'bg-[#18181b] text-[#d4d4d8] border-[#27272a] hover:border-[#3f3f46]'
          }`}
          title="Share real GPU screen stream"
        >
          <Video className="w-3.5 h-3.5 text-blue-400" />
          <span className="hidden sm:inline uppercase text-[10px] tracking-wider">
            {clientState.activeSource === 'REAL_NETWORK' ? 'Real GPU Stream' : 'Live Screen'}
          </span>
        </button>

        {/* Live FPS / Telemetry Pill */}
        <div className="hidden sm:flex items-center space-x-2 bg-[#0a0a0b] px-3 py-1 rounded border border-[#27272a] text-xs font-mono">
          <span className="flex items-center space-x-1.5 text-emerald-400 font-bold">
            <span className="w-1.5 h-1.5 rounded-full bg-emerald-500 shadow-[0_0_6px_rgba(16,185,129,0.8)]" />
            <span>{telemetry.fps} FPS</span>
          </span>
          <span className="text-[#3f3f46]">|</span>
          <span className="text-blue-400 font-semibold">{telemetry.totalLatencyMs}ms</span>
        </div>

        {/* Pairing Code Trigger */}
        <button
          type="button"
          onClick={() => setIsPairingOpen(true)}
          className="p-1.5 sm:px-3 sm:py-1 bg-[#18181b] hover:bg-[#27272a] text-white rounded border border-[#27272a] hover:border-blue-500/40 text-xs font-mono font-medium flex items-center space-x-1.5 transition-colors shadow-sm"
          title="Open Pairing Center"
        >
          <Lock className="w-3.5 h-3.5 text-blue-400" />
          <span className="hidden md:inline tracking-wider">PIN: {hostState.pairingPin}</span>
        </button>
      </div>

      {/* Mobile Tab Drawer for small screens */}
      <div className="flex lg:hidden fixed bottom-0 inset-x-0 z-40 bg-[#121214]/95 backdrop-blur-md border-t border-[#27272a] p-2 justify-around">
        {tabs.map((tab) => {
          const Icon = tab.icon;
          const isActive = activeTab === tab.id;
          return (
            <button
              key={tab.id}
              type="button"
              onClick={() => setActiveTab(tab.id as typeof activeTab)}
              className={`flex flex-col items-center space-y-1 p-1 rounded text-[10px] font-medium uppercase tracking-wider ${
                isActive ? 'text-blue-400 font-bold' : 'text-[#71717a]'
              }`}
            >
              <Icon className="w-4 h-4" />
              <span>{tab.label.split(' ')[0]}</span>
            </button>
          );
        })}
      </div>

      {/* Pairing Modal */}
      <PairingModal isOpen={isPairingOpen} onClose={() => setIsPairingOpen(false)} />
    </header>
  );
};

