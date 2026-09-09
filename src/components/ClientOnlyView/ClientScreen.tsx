/**
 * ClientScreen Component
 * Complete tablet / iPad second screen client implementation with Professional Polish design.
 * Supports:
 * - 100% Viewport Full Screen Mode (ESC or Floating menu to exit)
 * - Orientation Switcher (Landscape / Portrait)
 * - Scaling Modes (Contain, Cover, Native 100%)
 * - Auto-hiding Floating Menu for touch/mobile devices
 * - PIN Pairing workflow if disconnected
 * - Real Screen capture source switch (WebRTC / getDisplayMedia)
 */

import React, { useState, useEffect } from 'react';
import { useSecondScreen } from '../../context/SecondScreenContext';
import { DisplaySurface } from './DisplaySurface';
import {
  Smartphone,
  Tablet,
  Maximize,
  Minimize,
  RotateCw,
  WifiOff,
  ShieldCheck,
  AlertCircle,
  Video
} from 'lucide-react';
import { Platform } from '../../types';
import { NetworkState } from '../../types/protocol';

export const ClientScreen: React.FC = () => {
  const {
    clientState,
    setClientPlatform,
    setClientOrientation,
    setClientScalingMode,
    setActiveSource,
    toggleFullScreen,
    connectClientToHost,
    disconnectClient,
    triggerRealScreenCapture,
    stopRealScreenCapture,
    hostState,
    telemetry,
  } = useSecondScreen();

  const [pinInput, setPinInput] = useState('');
  const [pinError, setPinError] = useState(false);
  const [showOverlayMenu, setShowOverlayMenu] = useState(true);
  const [overlayTimeout, setOverlayTimeout] = useState<number | null>(null);

  // Auto-hide floating menu after 4 seconds of inactivity
  const handleInteraction = () => {
    setShowOverlayMenu(true);
    if (overlayTimeout) clearTimeout(overlayTimeout);
    const timeout = window.setTimeout(() => {
      setShowOverlayMenu(false);
    }, 4000);
    setOverlayTimeout(timeout);
  };

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && clientState.fullScreenActive) {
        toggleFullScreen();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [clientState.fullScreenActive, toggleFullScreen]);

  const handlePairSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    setPinError(false);
    const success = connectClientToHost(pinInput);
    if (!success) {
      setPinError(true);
    } else {
      setPinInput('');
    }
  };

  // If Client is disconnected or pairing is needed
  if (clientState.networkState === NetworkState.DISCONNECTED) {
    return (
      <div className="flex-1 p-6 flex items-center justify-center bg-[#0a0a0b]">
        <div className="max-w-md w-full bg-[#0f0f11] border border-[#27272a] rounded-lg p-6 shadow-2xl">
          <div className="flex items-center justify-between pb-4 border-b border-[#27272a]">
            <div className="flex items-center space-x-3">
              <div className="p-2.5 bg-blue-600/10 rounded text-blue-400 border border-blue-600/20">
                {clientState.platform === Platform.IOS_IPADOS ? <Tablet className="w-5 h-5" /> : <Smartphone className="w-5 h-5" />}
              </div>
              <div>
                <h3 className="font-bold text-white text-base uppercase tracking-wide">SecondScreen Client</h3>
                <p className="text-xs text-[#71717a] font-mono">Pair tablet receiver to PC/Mac Host</p>
              </div>
            </div>
            {/* Platform Selector */}
            <div className="flex bg-[#0a0a0b] p-1 rounded border border-[#27272a] text-xs font-mono">
              <button
                type="button"
                onClick={() => setClientPlatform(Platform.IOS_IPADOS)}
                className={`px-2.5 py-1 rounded font-bold transition-all uppercase ${
                  clientState.platform === Platform.IOS_IPADOS ? 'bg-blue-600 text-white shadow' : 'text-zinc-400 hover:text-white'
                }`}
              >
                iPadOS
              </button>
              <button
                type="button"
                onClick={() => setClientPlatform(Platform.ANDROID)}
                className={`px-2.5 py-1 rounded font-bold transition-all uppercase ${
                  clientState.platform === Platform.ANDROID ? 'bg-emerald-600 text-white shadow' : 'text-zinc-400 hover:text-white'
                }`}
              >
                Android
              </button>
            </div>
          </div>

          <form onSubmit={handlePairSubmit} className="mt-6 space-y-4">
            <div>
              <label className="block text-[11px] font-mono font-bold text-zinc-400 uppercase tracking-wider mb-2">
                Enter Host 6-Digit PIN
              </label>
              <div className="relative">
                <input
                  type="text"
                  maxLength={6}
                  value={pinInput}
                  onChange={(e) => setPinInput(e.target.value.replace(/\D/g, ''))}
                  placeholder={`e.g. ${hostState.pairingPin}`}
                  className="w-full bg-[#0a0a0b] border border-[#27272a] focus:border-blue-500 rounded py-3 px-4 text-center font-mono text-2xl tracking-[0.4em] text-white focus:outline-none focus:ring-1 focus:ring-blue-500"
                />
              </div>
              {pinError && (
                <p className="text-rose-400 text-xs mt-2 flex items-center space-x-1.5 font-mono">
                  <AlertCircle className="w-3.5 h-3.5" /> <span>Incorrect PIN. Host PIN is {hostState.pairingPin}</span>
                </p>
              )}
            </div>

            <div className="bg-[#0a0a0b] rounded p-3.5 border border-[#27272a] text-xs font-mono space-y-1.5 text-[#71717a]">
              <div className="flex justify-between">
                <span>Discovered Host:</span>
                <strong className="text-zinc-200">{hostState.hostName} ({hostState.ipAddress})</strong>
              </div>
              <div className="flex justify-between">
                <span>Decoder:</span>
                <strong className="text-blue-400">{clientState.decoder}</strong>
              </div>
            </div>

            <button
              type="submit"
              disabled={pinInput.length !== 6}
              className="w-full py-2.5 px-4 bg-blue-600 hover:bg-blue-500 disabled:opacity-40 disabled:cursor-not-allowed text-white font-semibold rounded transition-all shadow flex items-center justify-center space-x-2 text-xs uppercase tracking-wider cursor-pointer"
            >
              <ShieldCheck className="w-4 h-4" /> <span>Connect & Start Stream</span>
            </button>
          </form>
        </div>
      </div>
    );
  }

  // Active Streaming Tablet View
  return (
    <div
      id="client-screen-container"
      onMouseMove={handleInteraction}
      onTouchStart={handleInteraction}
      className={`relative w-full flex flex-col bg-black transition-all ${
        clientState.fullScreenActive ? 'fixed inset-0 z-50 h-screen' : 'h-[calc(100vh-4rem)] p-4'
      }`}
    >
      {/* Top Device Bezel Frame */}
      {!clientState.fullScreenActive && (
        <div className="bg-[#0f0f11] border border-[#27272a] rounded-t-lg px-4 py-2 flex items-center justify-between text-xs text-[#d4d4d8]">
          <div className="flex items-center space-x-2">
            <span className="w-2 h-2 rounded-full bg-emerald-500" />
            <span className="font-bold text-white uppercase text-xs tracking-wide">{clientState.clientName}</span>
            <span className="text-zinc-500 font-mono text-[10px]">({clientState.platform})</span>
          </div>

          <div className="flex items-center space-x-3">
            {/* Source Switcher */}
            <div className="flex bg-[#0a0a0b] p-0.5 rounded border border-[#27272a] text-[11px] font-mono">
              <button
                type="button"
                onClick={() => {
                  stopRealScreenCapture();
                  setActiveSource('SIMULATION');
                }}
                className={`px-2 py-0.5 rounded font-bold uppercase transition-colors ${
                  clientState.activeSource === 'SIMULATION' ? 'bg-blue-600 text-white' : 'text-zinc-400 hover:text-white'
                }`}
              >
                DaVinci Scopes
              </button>
              <button
                type="button"
                onClick={triggerRealScreenCapture}
                className={`px-2 py-0.5 rounded font-bold uppercase flex items-center space-x-1 transition-colors ${
                  clientState.activeSource === 'REAL_NETWORK' ? 'bg-emerald-600 text-white' : 'text-zinc-400 hover:text-white'
                }`}
              >
                <Video className="w-3 h-3" /> <span>Real GPU Capture</span>
              </button>
            </div>

            {/* Orientation Switcher */}
            <button
              type="button"
              onClick={() => setClientOrientation(clientState.orientation === 'LANDSCAPE' ? 'PORTRAIT' : 'LANDSCAPE')}
              className="px-2 py-1 bg-[#18181b] hover:bg-[#27272a] text-zinc-300 rounded border border-[#27272a] flex items-center space-x-1 font-mono text-xs cursor-pointer"
              title="Toggle Portrait/Landscape"
            >
              <RotateCw className="w-3 h-3" />
              <span className="hidden sm:inline uppercase text-[10px]">{clientState.orientation}</span>
            </button>

            {/* Scaling mode */}
            <select
              value={clientState.scalingMode}
              onChange={(e) => setClientScalingMode(e.target.value as 'CONTAIN' | 'COVER' | 'NATIVE_100')}
              aria-label="Mode de mise à l'échelle"
              className="bg-[#0a0a0b] border border-[#27272a] text-zinc-300 text-xs font-mono rounded px-2 py-1 focus:outline-none"
            >
              <option value="CONTAIN">Fit (Contain)</option>
              <option value="COVER">Fill (Cover)</option>
              <option value="NATIVE_100">100% Pixel Match</option>
            </select>

            {/* Fullscreen Button */}
            <button
              type="button"
              onClick={toggleFullScreen}
              className="px-2.5 py-1 bg-blue-600 hover:bg-blue-500 text-white rounded font-bold text-xs uppercase flex items-center space-x-1 shadow cursor-pointer"
            >
              <Maximize className="w-3 h-3" />
              <span>Full Screen</span>
            </button>
          </div>
        </div>
      )}

      {/* Main Display Surface Surface Container */}
      <div
        className={`relative flex-1 bg-[#0a0a0b] overflow-hidden flex items-center justify-center ${
          !clientState.fullScreenActive ? 'border-x border-b border-[#27272a] rounded-b-lg' : ''
        }`}
      >
        <div
          className={`w-full h-full transition-all duration-300 ${
            clientState.orientation === 'PORTRAIT' ? 'max-w-[720px] aspect-[9/16]' : 'w-full h-full'
          }`}
        >
          <DisplaySurface interactive={true} />
        </div>

        {/* Floating Auto-Hiding Overlay Menu in Fullscreen */}
        <div
          className={`absolute bottom-6 left-1/2 -translate-x-1/2 z-30 transition-all duration-300 ${
            showOverlayMenu || !clientState.fullScreenActive ? 'opacity-100 translate-y-0' : 'opacity-0 translate-y-6 pointer-events-none'
          }`}
        >
          <div className="bg-[#0f0f11]/95 backdrop-blur-md border border-[#27272a] rounded-full px-4 py-1.5 shadow-2xl flex items-center space-x-3 text-xs font-mono text-[#d4d4d8]">
            <span className="flex items-center space-x-1.5 text-emerald-400">
              <span className="w-2 h-2 rounded-full bg-emerald-400" />
              <span>{telemetry.fps} FPS</span>
            </span>
            <span className="text-zinc-600">|</span>
            <span className="text-blue-400">{telemetry.totalLatencyMs} ms</span>
            <span className="text-zinc-600">|</span>

            {/* Scaling switch */}
            <button
              type="button"
              onClick={() => {
                const next = clientState.scalingMode === 'CONTAIN' ? 'COVER' : clientState.scalingMode === 'COVER' ? 'NATIVE_100' : 'CONTAIN';
                setClientScalingMode(next);
              }}
              className="hover:text-white transition-colors cursor-pointer text-[11px]"
            >
              Scale: {clientState.scalingMode}
            </button>
            <span className="text-zinc-600">|</span>

            {/* Orientation */}
            <button
              type="button"
              onClick={() => setClientOrientation(clientState.orientation === 'LANDSCAPE' ? 'PORTRAIT' : 'LANDSCAPE')}
              className="hover:text-white flex items-center space-x-1 cursor-pointer"
            >
              <RotateCw className="w-3 h-3" />
            </button>
            <span className="text-zinc-600">|</span>

            {/* Exit Fullscreen or Disconnect */}
            {clientState.fullScreenActive ? (
              <button
                type="button"
                onClick={toggleFullScreen}
                className="bg-[#18181b] hover:bg-[#27272a] px-2.5 py-0.5 rounded-full text-zinc-200 flex items-center space-x-1 border border-[#27272a] cursor-pointer text-[11px]"
              >
                <Minimize className="w-3 h-3" /> <span>Exit Fullscreen (ESC)</span>
              </button>
            ) : (
              <button
                type="button"
                onClick={disconnectClient}
                className="bg-rose-900/20 hover:bg-rose-900/40 text-rose-400 border border-rose-800/40 px-2.5 py-0.5 rounded-full flex items-center space-x-1 cursor-pointer text-[11px]"
              >
                <WifiOff className="w-3 h-3" /> <span>Disconnect</span>
              </button>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};
