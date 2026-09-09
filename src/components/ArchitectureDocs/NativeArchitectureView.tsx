/**
 * NativeArchitectureView Component
 * Comprehensive native codebase and driver inspector with Professional Polish design:
 * Windows (IddCx / DXGI / NVENC), macOS (CGVirtualDisplay / ScreenCaptureKit / VideoToolbox),
 * Android (Kotlin MediaCodec / SurfaceView), and iOS/iPadOS (Swift Metal / VideoToolbox).
 */

import React, { useState } from 'react';
import { NATIVE_SOURCE_FILES } from '../../data/nativeSourceCode';
import {
  FileCode,
  CheckCircle2,
  Copy,
  ShieldAlert,
  Code2,
  FolderOpen
} from 'lucide-react';

export const NativeArchitectureView: React.FC = () => {
  const [selectedPlatform, setSelectedPlatform] = useState<'ALL' | 'WINDOWS' | 'MACOS' | 'ANDROID' | 'IOS'>('WINDOWS');
  const [selectedFileIndex, setSelectedFileIndex] = useState(0);
  const [copied, setCopied] = useState(false);

  const filteredFiles = selectedPlatform === 'ALL'
    ? NATIVE_SOURCE_FILES
    : NATIVE_SOURCE_FILES.filter((f) => f.platform === selectedPlatform);

  const activeFile = filteredFiles[selectedFileIndex] || filteredFiles[0];

  const handleCopyCode = () => {
    if (activeFile) {
      navigator.clipboard.writeText(activeFile.code);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    }
  };

  return (
    <div className="flex-1 p-6 bg-[#0a0a0b] text-[#d4d4d8] overflow-y-auto">
      <div className="max-w-7xl mx-auto space-y-6">
        {/* Header */}
        <div className="bg-[#0f0f11] border border-[#27272a] rounded-lg p-5 shadow-xl flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div className="flex items-center space-x-3.5">
            <div className="p-2.5 bg-blue-600/10 rounded text-blue-400 border border-blue-600/20">
              <Code2 className="w-5 h-5" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h2 className="text-base font-bold text-white uppercase tracking-wide">
                  Native Driver & Core Codebase Explorer
                </h2>
                <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-blue-900/30 text-blue-400 border border-blue-800/50 uppercase tracking-wider">
                  C++ • Swift • Kotlin
                </span>
              </div>
              <p className="text-xs text-[#71717a] mt-0.5 font-mono">
                Authentic driver source code, GPU capture pipelines, and hardware decoders for Windows, macOS, Android & iOS.
              </p>
            </div>
          </div>

          {/* Platform filter */}
          <div className="flex bg-[#0a0a0b] p-1 rounded border border-[#27272a] text-xs font-mono">
            {(['WINDOWS', 'MACOS', 'ANDROID', 'IOS', 'ALL'] as const).map((p) => (
              <button
                key={p}
                type="button"
                onClick={() => {
                  setSelectedPlatform(p);
                  setSelectedFileIndex(0);
                }}
                className={`px-3 py-1 text-xs font-bold rounded transition-all uppercase ${
                  selectedPlatform === p
                    ? 'bg-blue-600 text-white shadow'
                    : 'text-zinc-400 hover:text-white'
                }`}
              >
                {p}
              </button>
            ))}
          </div>
        </div>

        {/* Code Explorer Layout */}
        <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
          {/* File Tree Sidebar */}
          <div className="lg:col-span-4 bg-[#0f0f11] border border-[#27272a] rounded-lg p-4 shadow-xl space-y-3">
            <div className="flex items-center justify-between text-xs font-bold text-[#71717a] uppercase tracking-wider pb-2 border-b border-[#27272a]">
              <span className="flex items-center space-x-1.5 text-[10px]">
                <FolderOpen className="w-3.5 h-3.5 text-blue-500" />
                <span>Native Files ({filteredFiles.length})</span>
              </span>
            </div>

            <div className="space-y-1.5">
              {filteredFiles.map((file, idx) => (
                <button
                  key={file.path}
                  type="button"
                  onClick={() => setSelectedFileIndex(idx)}
                  className={`w-full p-3 rounded border text-left transition-all flex items-start space-x-2.5 ${
                    activeFile?.path === file.path
                      ? 'bg-blue-600/10 border-blue-500/60 text-white shadow'
                      : 'bg-[#18181b] border-[#27272a] text-zinc-400 hover:border-zinc-700 hover:text-white'
                  }`}
                >
                  <FileCode className={`w-4 h-4 shrink-0 mt-0.5 ${
                    file.language === 'cpp' ? 'text-blue-400' :
                    file.language === 'swift' ? 'text-amber-400' :
                    file.language === 'kotlin' ? 'text-purple-400' : 'text-emerald-400'
                  }`} />
                  <div className="overflow-hidden">
                    <div className="font-mono text-xs truncate font-semibold">{file.path.split('/').pop()}</div>
                    <div className="text-[10px] text-zinc-500 truncate mt-0.5">{file.category} • {file.platform}</div>
                  </div>
                </button>
              ))}
            </div>

            {/* Security & Driver Signing Note */}
            <div className="mt-4 p-3 bg-amber-900/10 border border-amber-800/30 rounded text-amber-300 text-xs space-y-1">
              <div className="font-semibold flex items-center space-x-1.5 text-[11px] uppercase tracking-wider">
                <ShieldAlert className="w-3.5 h-3.5 text-amber-400" />
                <span>OS Driver Signing Requirement</span>
              </div>
              <p className="text-[10px] text-amber-300/80 leading-relaxed font-mono">
                Windows IddCx requires Windows Driver Kit (WDK 10) and WHQL / Test-Signing mode enabled.
                macOS DriverKit requires Apple Developer System Extension entitlement with TCC screen recording access.
              </p>
            </div>
          </div>

          {/* Main Code Viewer */}
          <div className="lg:col-span-8 bg-[#0f0f11] border border-[#27272a] rounded-lg p-5 shadow-xl flex flex-col space-y-4">
            {activeFile ? (
              <>
                <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 pb-3 border-b border-[#27272a]">
                  <div>
                    <span className="font-mono text-xs font-bold text-blue-400 block">{activeFile.path}</span>
                    <span className="text-xs text-[#71717a]">{activeFile.description}</span>
                  </div>

                  <button
                    type="button"
                    onClick={handleCopyCode}
                    className="self-start sm:self-auto px-3 py-1.5 bg-[#18181b] hover:bg-[#27272a] text-zinc-200 rounded text-xs font-mono flex items-center space-x-1.5 border border-[#27272a] transition-colors cursor-pointer"
                  >
                    {copied ? <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                    <span>{copied ? 'Copied!' : 'Copy Code'}</span>
                  </button>
                </div>

                {/* Code Block */}
                <div className="bg-[#0a0a0b] rounded p-4 border border-[#27272a] overflow-x-auto max-h-[560px] text-xs font-mono leading-relaxed text-zinc-200">
                  <pre>{activeFile.code}</pre>
                </div>
              </>
            ) : (
              <div className="p-12 text-center text-zinc-500 text-sm font-mono">Select a file from the sidebar</div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};

