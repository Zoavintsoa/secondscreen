/**
 * DisplaySurface Component
 * Implements clean polymorphic abstraction between:
 * 1. SimulationSource: Interactive DaVinci Resolve color scopes & creative canvas (Professional Polish design)
 * 2. NetworkVideoSource: Real WebRTC / HTMLMediaStream GPU frame stream
 */

import React, { useRef, useEffect, useState } from 'react';
import { useSecondScreen } from '../../context/SecondScreenContext';
import { networkStreamService } from '../../services/networkStreamService';
import { Radio } from 'lucide-react';

interface DisplaySurfaceProps {
  interactive?: boolean;
  className?: string;
}

export const DisplaySurface: React.FC<DisplaySurfaceProps> = ({ interactive = true, className = '' }) => {
  const {
    clientState,
    daVinciState,
    telemetry,
    hostState,
  } = useSecondScreen();

  const videoRef = useRef<HTMLVideoElement>(null);
  const [touchPoint, setTouchPoint] = useState<{ x: number; y: number } | null>(null);

  // Bind real MediaStream if available in NetworkVideoSource mode
  useEffect(() => {
    if (clientState.activeSource === 'REAL_NETWORK' && videoRef.current) {
      const realStream = networkStreamService.getRealStream();
      if (realStream) {
        videoRef.current.srcObject = realStream;
        videoRef.current.play().catch(console.warn);
      }
    }
  }, [clientState.activeSource]);

  // Handle pointer touch serialization to network protocol
  const handlePointerDown = (e: React.PointerEvent<HTMLDivElement>) => {
    if (!interactive) return;
    const rect = e.currentTarget.getBoundingClientRect();
    const normX = (e.clientX - rect.left) / rect.width;
    const normY = (e.clientY - rect.top) / rect.height;

    setTouchPoint({ x: e.clientX - rect.left, y: e.clientY - rect.top });

    networkStreamService.sendInputEvent({
      type: 'POINTER_DOWN',
      x: normX,
      y: normY,
      button: e.button,
      pressure: e.pressure || 1.0,
    });
  };

  const handlePointerMove = (e: React.PointerEvent<HTMLDivElement>) => {
    if (!touchPoint || !interactive) return;
    const rect = e.currentTarget.getBoundingClientRect();
    const normX = (e.clientX - rect.left) / rect.width;
    const normY = (e.clientY - rect.top) / rect.height;

    setTouchPoint({ x: e.clientX - rect.left, y: e.clientY - rect.top });

    networkStreamService.sendInputEvent({
      type: 'POINTER_MOVE',
      x: normX,
      y: normY,
      button: e.button,
      pressure: e.pressure || 1.0,
    });
  };

  const handlePointerUp = (e: React.PointerEvent<HTMLDivElement>) => {
    if (!interactive) return;
    const rect = e.currentTarget.getBoundingClientRect();
    const normX = (e.clientX - rect.left) / rect.width;
    const normY = (e.clientY - rect.top) / rect.height;

    setTouchPoint(null);

    networkStreamService.sendInputEvent({
      type: 'POINTER_UP',
      x: normX,
      y: normY,
      button: e.button,
    });
  };

  // Render Real Screen Capture Source
  if (clientState.activeSource === 'REAL_NETWORK') {
    return (
      <div
        id="display-surface-real-video"
        className={`relative w-full h-full bg-black flex items-center justify-center overflow-hidden select-none ${className}`}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
      >
        <video
          ref={videoRef}
          autoPlay
          playsInline
          muted
          className={`w-full h-full ${
            clientState.scalingMode === 'CONTAIN'
              ? 'object-contain'
              : clientState.scalingMode === 'COVER'
              ? 'object-cover'
              : 'object-none'
          }`}
        />
        {/* Real Stream Live Overlay Badge */}
        <div className="absolute top-4 left-4 z-20 flex items-center space-x-2 bg-[#121214]/90 backdrop-blur-md border border-blue-500/40 rounded-full px-3 py-1.5 shadow-lg">
          <span className="relative flex h-2 w-2">
            <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>
            <span className="relative inline-flex rounded-full h-2 w-2 bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.8)]"></span>
          </span>
          <span className="text-[10px] font-mono font-bold text-white tracking-widest uppercase">
            REAL GPU STREAM (WEBRTC / NVENC)
          </span>
        </div>

        {/* Touch digitizer ripple */}
        {touchPoint && (
          <div
            className="absolute w-8 h-8 -ml-4 -mt-4 rounded-full border-2 border-blue-400 bg-blue-400/20 pointer-events-none animate-ping"
            style={{ left: touchPoint.x, top: touchPoint.y }}
          />
        )}
      </div>
    );
  }

  // Render High-Fidelity SimulationSource (DaVinci Resolve Creative Video Scopes)
  return (
    <div
      id="display-surface-simulation"
      className={`relative w-full h-full bg-[#121214] rounded-lg border-2 border-blue-600/50 shadow-[0_0_40px_rgba(37,99,235,0.15)] flex flex-col overflow-hidden select-none text-[#d4d4d8] ${className}`}
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onPointerUp={handlePointerUp}
    >
      {/* DaVinci Resolve Window Header Simulation */}
      <div className="bg-[#27272a] px-4 py-1.5 flex items-center justify-between shrink-0">
        <div className="text-[10px] text-[#a1a1aa] font-medium tracking-wide">
          DaVinci Resolve - Scopes (Display 2 Virtual Monitor)
        </div>
        <div className="flex items-center space-x-2">
          <div className="w-2 h-2 rounded-full bg-[#3f3f46]"></div>
          <div className="w-2 h-2 rounded-full bg-[#3f3f46]"></div>
        </div>
      </div>

      {/* 2x2 Scope Grid */}
      <div className="flex-1 grid grid-cols-2 grid-rows-2 gap-1 p-1 bg-black">
        {/* Scope 1: RGB Parade */}
        <div className="bg-[#18181b] flex flex-col items-center justify-center relative border border-white/5 p-2">
          <div className="absolute top-1.5 left-2 text-[8px] text-zinc-500 uppercase font-mono tracking-wider">
            RGB Parade
          </div>
          <div className="w-4/5 h-2/3 border-b border-l border-zinc-700 flex items-end space-x-1 p-1">
            {/* Red Bar */}
            <div
              className="w-1/3 bg-red-500/20 border-t-2 border-red-500/80 transition-all duration-75 rounded-t-sm"
              style={{
                height: `${Math.min(95, Math.max(15, 65 + daVinciState.lift.r * 25 + daVinciState.gain.r * 20))}%`,
              }}
            />
            {/* Green Bar */}
            <div
              className="w-1/3 bg-green-500/20 border-t-2 border-green-500/80 transition-all duration-75 rounded-t-sm"
              style={{
                height: `${Math.min(95, Math.max(15, 55 + daVinciState.lift.g * 25 + daVinciState.gain.g * 20))}%`,
              }}
            />
            {/* Blue Bar */}
            <div
              className="w-1/3 bg-blue-500/20 border-t-2 border-blue-500/80 transition-all duration-75 rounded-t-sm"
              style={{
                height: `${Math.min(95, Math.max(15, 60 + daVinciState.lift.b * 25 + daVinciState.gain.b * 20))}%`,
              }}
            />
          </div>
        </div>

        {/* Scope 2: Vectorscope */}
        <div className="bg-[#18181b] flex flex-col items-center justify-center relative border border-white/5">
          <div className="absolute top-1.5 left-2 text-[8px] text-zinc-500 uppercase font-mono tracking-wider">
            Vectorscope
          </div>
          <div className="w-20 h-20 rounded-full border border-zinc-700 flex items-center justify-center relative">
            <div className="w-px h-full bg-zinc-800 rotate-45"></div>
            <div className="w-px h-full bg-zinc-800 -rotate-45"></div>
            {/* Color trace dot */}
            <div
              className="w-3 h-3 bg-yellow-400 blur-[2px] rounded-full transition-transform duration-75 shadow-[0_0_8px_rgba(250,204,21,0.6)]"
              style={{
                transform: `translate(${daVinciState.colorTemperature * 0.4 + 12}px, ${-daVinciState.lift.r * 10 - 8}px) scale(${0.8 + daVinciState.saturation / 60})`,
              }}
            ></div>
          </div>
        </div>

        {/* Scope 3: Waveform */}
        <div className="bg-[#18181b] flex flex-col items-center justify-center relative border border-white/5">
          <div className="absolute top-1.5 left-2 text-[8px] text-zinc-500 uppercase font-mono tracking-wider">
            Waveform
          </div>
          <div className="w-4/5 h-2/3 flex items-center overflow-hidden relative">
            <div className="w-full h-1/2 border-y border-zinc-800 opacity-50"></div>
            <div
              className="absolute w-full h-[2px] bg-white/40 blur-[1px] transform transition-transform duration-75"
              style={{
                transform: `translateY(${(-daVinciState.gain.y * 15 - (daVinciState.contrast - 50) * 0.2)}px)`,
              }}
            ></div>
          </div>
        </div>

        {/* Scope 4: Histogram */}
        <div className="bg-[#18181b] flex flex-col items-center justify-center relative border border-white/5">
          <div className="absolute top-1.5 left-2 text-[8px] text-zinc-500 uppercase font-mono tracking-wider">
            Histogram
          </div>
          <div className="w-4/5 h-1/2 flex items-end space-x-[2px]">
            {Array.from({ length: 12 }).map((_, i) => {
              const heights = [20, 35, 60, 85, 70, 50, 40, 65, 80, 55, 30, 20];
              const h = heights[i % heights.length];
              const adjusted = Math.min(95, Math.max(10, h + (daVinciState.contrast - 50) * 0.3));
              return (
                <div
                  key={i}
                  className="flex-1 bg-zinc-600 rounded-t-xs transition-all duration-75"
                  style={{ height: `${adjusted}%` }}
                />
              );
            })}
          </div>
        </div>
      </div>

      {/* Footer Status Bar */}
      <div className="h-6 bg-[#121214] border-t border-[#27272a] px-3 flex items-center justify-between text-[10px] text-[#71717a] font-mono">
        <div>
          <span>Host: {hostState.hostName}</span>
          <span className="mx-2 text-[#3f3f46]">|</span>
          <span>Target: {clientState.clientName}</span>
        </div>
        <div className="flex items-center space-x-3">
          <span>FPS: <strong className="text-white">{telemetry.fps}</strong></span>
          <span>Latency: <strong className="text-emerald-400">{telemetry.totalLatencyMs}ms</strong></span>
        </div>
      </div>

      {/* Touch digitizer ripple */}
      {touchPoint && (
        <div
          className="absolute w-8 h-8 -ml-4 -mt-4 rounded-full border-2 border-blue-400 bg-blue-400/20 pointer-events-none animate-ping"
          style={{ left: touchPoint.x, top: touchPoint.y }}
        />
      )}
    </div>
  );
};

