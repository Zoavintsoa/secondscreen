/**
 * SecondScreen Master Application Context
 * Central state management connecting Host View, Client Tablet View,
 * Live Dual-Screen DaVinci Lab, Diagnostics, and Telemetry.
 */

import React, { createContext, useContext, useState, useEffect, useCallback, ReactNode } from 'react';
import {
  Platform,
  HostState,
  ClientState,
  DaVinciScopeState,
  TrustedClient,
  DiagnosticItem,
  FeatureStatus
} from '../types';
import {
  DisplayMode,
  VideoCodec,
  EncoderType,
  DecoderType,
  NetworkState,
  TelemetryPayload,
  QualityPreset,
  DisplayConfigPayload
} from '../types/protocol';
import { networkStreamService } from '../services/networkStreamService';

interface SecondScreenContextType {
  // Navigation & View Mode
  activeTab: 'HOST_VIEW' | 'CLIENT_VIEW' | 'DUAL_SCREEN_LAB' | 'DIAGNOSTICS' | 'ARCHITECTURE';
  setActiveTab: (tab: 'HOST_VIEW' | 'CLIENT_VIEW' | 'DUAL_SCREEN_LAB' | 'DIAGNOSTICS' | 'ARCHITECTURE') => void;

  // Host State
  hostState: HostState;
  setHostOS: (os: Platform.WINDOWS | Platform.MACOS) => void;
  toggleVirtualDisplay: () => void;
  setHostDisplayMode: (mode: DisplayMode) => void;
  setHostEncoder: (encoder: EncoderType) => void;
  setHostCodec: (codec: VideoCodec) => void;
  setHostQualityPreset: (preset: QualityPreset) => void;
  regeneratePairingPin: () => void;
  revokeTrustedClient: (clientId: string) => void;

  // Client State
  clientState: ClientState;
  setClientPlatform: (platform: Platform.ANDROID | Platform.IOS_IPADOS) => void;
  setClientOrientation: (orientation: 'LANDSCAPE' | 'PORTRAIT') => void;
  setClientScalingMode: (mode: 'CONTAIN' | 'COVER' | 'NATIVE_100') => void;
  setActiveSource: (source: 'SIMULATION' | 'REAL_NETWORK') => void;
  toggleFullScreen: () => void;
  connectClientToHost: (pin: string) => boolean;
  disconnectClient: () => void;
  triggerRealScreenCapture: () => Promise<boolean>;
  stopRealScreenCapture: () => void;

  // Telemetry
  telemetry: TelemetryPayload;
  telemetryHistory: { time: string; fps: number; latency: number; bitrate: number }[];

  // DaVinci Resolve Creative Workflow State
  daVinciState: DaVinciScopeState;
  setDaVinciScope: (scope: 'RGB_PARADE' | 'WAVEFORM' | 'VECTORSCOPE' | 'HISTOGRAM' | 'ALL_QUAD') => void;
  updateColorGrade: (params: Partial<DaVinciScopeState>) => void;
  toggleScopesOnDisplay2: () => void;
  resetColorGrade: () => void;

  // Diagnostics
  diagnostics: DiagnosticItem[];
  runFullDiagnosticCheck: () => void;
  isDiagnosticRunning: boolean;
}

const defaultTelemetry: TelemetryPayload = {
  fps: 0,
  bitrateMbps: 0,
  rttMs: 0,
  decodeLatencyMs: 0,
  renderLatencyMs: 0,
  totalLatencyMs: 0,
  packetLossPercent: 0,
  frameDrops: 0,
  jitterMs: 1.2,
  measured: false,
};

const SecondScreenContext = createContext<SecondScreenContextType | undefined>(undefined);

export const SecondScreenProvider: React.FC<{ children: ReactNode }> = ({ children }) => {
  const [activeTab, setActiveTab] = useState<'HOST_VIEW' | 'CLIENT_VIEW' | 'DUAL_SCREEN_LAB' | 'DIAGNOSTICS' | 'ARCHITECTURE'>('DUAL_SCREEN_LAB');

  // Host Configuration State
  const [hostState, setHostState] = useState<HostState>({
    os: Platform.WINDOWS,
    hostName: 'Studio-Workstation-Pro',
    ipAddress: '192.168.1.145',
    port: 9876,
    isServerRunning: false,
    virtualDisplayActive: false,
    displays: [
      {
        displayId: 1,
        displayName: 'Display 1 (Primary LG UltraFine 4K)',
        width: 3840,
        height: 2160,
        refreshRate: 60,
        orientation: 'LANDSCAPE',
        mode: DisplayMode.EXTEND,
        colorSpace: 'DCI-P3',
        dpiScale: 1.5,
        isVirtual: false,
      },
      {
        displayId: 2,
        displayName: 'Display 2 (SecondScreen Virtual Monitor)',
        width: 2560,
        height: 1600,
        refreshRate: 60,
        orientation: 'LANDSCAPE',
        mode: DisplayMode.EXTEND,
        colorSpace: 'Rec.709',
        dpiScale: 2.0,
        isVirtual: true,
      },
    ],
    activeDisplayId: 2,
    encoder: EncoderType.NVENC,
    selectedCodec: VideoCodec.H264,
    qualityPreset: QualityPreset.BALANCED,
    pairingPin: '482931',
    pinExpirySeconds: 180,
    trustedClients: [],
  });

  // Client Configuration State
  const [clientState, setClientState] = useState<ClientState>({
    platform: Platform.ANDROID,
    clientName: 'Android tablet (not connected)',
    ipAddress: 'Not measured',
    networkState: NetworkState.DISCONNECTED,
    connectedHost: null,
    decoder: DecoderType.MEDIACODEC,
    fullScreenActive: false,
    scalingMode: 'CONTAIN',
    orientation: 'LANDSCAPE',
    activeSource: 'SIMULATION',
    latencyMode: 'ULTRA_LOW',
  });

  // Telemetry State
  const [telemetry, setTelemetry] = useState<TelemetryPayload>(defaultTelemetry);
  const [telemetryHistory, setTelemetryHistory] = useState<{ time: string; fps: number; latency: number; bitrate: number }[]>([]);

  // DaVinci Resolve Scope & Color Grading State
  const [daVinciState, setDaVinciState] = useState<DaVinciScopeState>({
    activeScope: 'ALL_QUAD',
    sourceImage: 'cinema_raw_sample',
    lift: { r: 0.0, g: 0.0, b: 0.0, y: 0.0 },
    gamma: { r: 0.0, g: 0.0, b: 0.0, y: 0.0 },
    gain: { r: 0.0, g: 0.0, b: 0.0, y: 0.0 },
    saturation: 50,
    contrast: 50,
    colorTemperature: 0,
    scopesOnDisplay2: true,
  });

  // Diagnostic tests status
  const [isDiagnosticRunning, setIsDiagnosticRunning] = useState(false);
  const [diagnostics, setDiagnostics] = useState<DiagnosticItem[]>([
    {
      id: 'diag-1',
      name: 'Windows IddCx Virtual Display Driver',
      category: 'HOST_DRIVER',
      status: 'PENDING',
      measuredValue: 'SOURCE READY (PHYSICAL TEST: PENDING)',
      expectedValue: 'Active Virtual Monitor Handle (Display 2)',
      detail: 'C++ UMDF 2.0 driver and INF ready. Physical installation on Windows 10/11 host pending.',
      isRealMetric: false,
    },
    {
      id: 'diag-2',
      name: 'Windows DXGI Zero-Copy Frame Capture',
      category: 'CAPTURE',
      status: 'PENDING',
      measuredValue: 'SOURCE READY (PHYSICAL TEST: PENDING)',
      expectedValue: 'Direct3D 11 ID3D11Texture2D in VRAM',
      detail: 'DXGICaptureManager zero-copy grabber and display enumeration ready. Physical GPU validation pending.',
      isRealMetric: false,
    },
    {
      id: 'diag-3',
      name: 'Hardware Encoder (NVENC / QuickSync / AMF)',
      category: 'ENCODER',
      status: 'PENDING',
      measuredValue: 'SOURCE READY (PHYSICAL TEST: PENDING)',
      expectedValue: 'NVENC, QuickSync, or AMF low-latency CBR',
      detail: 'HardwareEncoderFactory and dynamic loaders ready. Physical GPU detection pending.',
      isRealMetric: false,
    },
    {
      id: 'diag-4',
      name: 'Native WinSock2 Transport (TCP 9876 + Port 9878)',
      category: 'NETWORK',
      status: 'PENDING',
      measuredValue: 'SOURCE READY (PHYSICAL TEST: PENDING)',
      expectedValue: 'RTT < 16ms, Zero packet framing errors',
      detail: 'WinSock2 dual-channel network engine and discovery ready. Physical LAN connection pending.',
      isRealMetric: false,
    },
    {
      id: 'diag-5',
      name: 'Android MediaCodec Hardware Decoder',
      category: 'DECODER',
      status: 'PENDING',
      measuredValue: 'SOURCE READY (PHYSICAL TEST: PENDING)',
      expectedValue: 'Direct hardware Surface rendering',
      detail: 'Kotlin MediaCodec decoder, SurfaceView, and SecondScreenClient ready. Tablet test pending.',
    },
  ]);

  // Listen to network stream telemetry
  useEffect(() => {
    const unsub = networkStreamService.onTelemetry((t) => {
      setTelemetry(t);
      setTelemetryHistory((prev) => {
        const next = [...prev.slice(1), {
          time: new Date().toLocaleTimeString().slice(3, 8),
          fps: t.fps,
          latency: t.totalLatencyMs,
          bitrate: t.bitrateMbps,
        }];
        return next;
      });
    });
    return () => unsub();
  }, []);

  const setHostOS = useCallback((os: Platform.WINDOWS | Platform.MACOS) => {
    setHostState((prev) => ({
      ...prev,
      os,
      encoder: os === Platform.WINDOWS ? EncoderType.NVENC : EncoderType.VIDEOTOOLBOX,
      hostName: os === Platform.WINDOWS ? 'Studio-PC-Workstation' : 'Mac-Studio-M2-Max',
    }));
  }, []);

  const toggleVirtualDisplay = useCallback(() => {
    setHostState((prev) => ({
      ...prev,
      virtualDisplayActive: !prev.virtualDisplayActive,
    }));
  }, []);

  const setHostDisplayMode = useCallback((mode: DisplayMode) => {
    setHostState((prev) => ({
      ...prev,
      displays: prev.displays.map((d) => (d.isVirtual ? { ...d, mode } : d)),
    }));
  }, []);

  const setHostEncoder = useCallback((encoder: EncoderType) => {
    setHostState((prev) => ({ ...prev, encoder }));
  }, []);

  const setHostCodec = useCallback((codec: VideoCodec) => {
    setHostState((prev) => ({ ...prev, selectedCodec: codec }));
  }, []);

  const setHostQualityPreset = useCallback((preset: QualityPreset) => {
    setHostState((prev) => ({ ...prev, qualityPreset: preset }));
  }, []);

  const regeneratePairingPin = useCallback(() => {
    const newPin = Math.floor(100000 + Math.random() * 900000).toString();
    setHostState((prev) => ({
      ...prev,
      pairingPin: newPin,
      pinExpirySeconds: 180,
    }));
  }, []);

  const revokeTrustedClient = useCallback((clientId: string) => {
    setHostState((prev) => ({
      ...prev,
      trustedClients: prev.trustedClients.filter((c) => c.id !== clientId),
    }));
  }, []);

  const setClientPlatform = useCallback((platform: Platform.ANDROID | Platform.IOS_IPADOS) => {
    setClientState((prev) => ({
      ...prev,
      platform,
      clientName: platform === Platform.IOS_IPADOS ? 'iPad Pro 12.9 (M2 Studio)' : 'Samsung Galaxy Tab S9 Ultra',
      decoder: platform === Platform.IOS_IPADOS ? DecoderType.VIDEOTOOLBOX : DecoderType.MEDIACODEC,
    }));
  }, []);

  const setClientOrientation = useCallback((orientation: 'LANDSCAPE' | 'PORTRAIT') => {
    setClientState((prev) => ({ ...prev, orientation }));
  }, []);

  const setClientScalingMode = useCallback((mode: 'CONTAIN' | 'COVER' | 'NATIVE_100') => {
    setClientState((prev) => ({ ...prev, scalingMode: mode }));
  }, []);

  const setActiveSource = useCallback((source: 'SIMULATION' | 'REAL_NETWORK') => {
    setClientState((prev) => ({ ...prev, activeSource: source }));
  }, []);

  const toggleFullScreen = useCallback(() => {
    setClientState((prev) => ({ ...prev, fullScreenActive: !prev.fullScreenActive }));
  }, []);

  const connectClientToHost = useCallback((pin: string): boolean => {
    const success = networkStreamService.attemptPairing(pin, hostState.pairingPin, clientState.clientName);
    if (success) {
      setClientState((prev) => ({
        ...prev,
        networkState: NetworkState.STREAMING,
        connectedHost: {
          name: hostState.hostName,
          ip: hostState.ipAddress,
          os: hostState.os,
        },
      }));
    }
    return success;
  }, [hostState.pairingPin, hostState.hostName, hostState.ipAddress, hostState.os, clientState.clientName]);

  const disconnectClient = useCallback(() => {
    networkStreamService.disconnect();
    setClientState((prev) => ({
      ...prev,
      networkState: NetworkState.DISCONNECTED,
      connectedHost: null,
      activeSource: 'SIMULATION',
    }));
  }, []);

  const triggerRealScreenCapture = useCallback(async (): Promise<boolean> => {
    const stream = await networkStreamService.requestRealScreenCapture();
    if (stream) {
      setClientState((prev) => ({
        ...prev,
        activeSource: 'REAL_NETWORK',
        networkState: NetworkState.STREAMING,
      }));
      return true;
    }
    return false;
  }, []);

  const stopRealScreenCapture = useCallback(() => {
    networkStreamService.stopRealScreenCapture();
    setClientState((prev) => ({
      ...prev,
      activeSource: 'SIMULATION',
    }));
  }, []);

  const setDaVinciScope = useCallback((scope: 'RGB_PARADE' | 'WAVEFORM' | 'VECTORSCOPE' | 'HISTOGRAM' | 'ALL_QUAD') => {
    setDaVinciState((prev) => ({ ...prev, activeScope: scope }));
  }, []);

  const updateColorGrade = useCallback((params: Partial<DaVinciScopeState>) => {
    setDaVinciState((prev) => ({ ...prev, ...params }));
  }, []);

  const toggleScopesOnDisplay2 = useCallback(() => {
    setDaVinciState((prev) => ({ ...prev, scopesOnDisplay2: !prev.scopesOnDisplay2 }));
  }, []);

  const resetColorGrade = useCallback(() => {
    setDaVinciState((prev) => ({
      ...prev,
      lift: { r: 0, g: 0, b: 0, y: 0 },
      gamma: { r: 0, g: 0, b: 0, y: 0 },
      gain: { r: 0, g: 0, b: 0, y: 0 },
      saturation: 50,
      contrast: 50,
      colorTemperature: 0,
    }));
  }, []);

  const runFullDiagnosticCheck = useCallback(() => {
    setIsDiagnosticRunning(true);
    setTimeout(() => {
      setIsDiagnosticRunning(false);
    }, 800);
  }, []);

  return (
    <SecondScreenContext.Provider
      value={{
        activeTab,
        setActiveTab,
        hostState,
        setHostOS,
        toggleVirtualDisplay,
        setHostDisplayMode,
        setHostEncoder,
        setHostCodec,
        setHostQualityPreset,
        regeneratePairingPin,
        revokeTrustedClient,
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
        telemetry,
        telemetryHistory,
        daVinciState,
        setDaVinciScope,
        updateColorGrade,
        toggleScopesOnDisplay2,
        resetColorGrade,
        diagnostics,
        runFullDiagnosticCheck,
        isDiagnosticRunning,
      }}
    >
      {children}
    </SecondScreenContext.Provider>
  );
};

export const useSecondScreen = () => {
  const context = useContext(SecondScreenContext);
  if (!context) {
    throw new Error('useSecondScreen must be used within a SecondScreenProvider');
  }
  return context;
};
