/**
 * SecondScreen Application Shared Types
 */

import {
  DisplayMode,
  VideoCodec,
  EncoderType,
  DecoderType,
  NetworkState,
  TelemetryPayload,
  DisplayConfigPayload,
  QualityPreset
} from './protocol';

export enum Platform {
  WINDOWS = 'WINDOWS',
  MACOS = 'MACOS',
  ANDROID = 'ANDROID',
  IOS_IPADOS = 'IOS_IPADOS',
}

/**
 * Strict status categorization mandated by rule:
 * IMPLEMENTED: Real functioning code tested end-to-end
 * PROTOTYPE: Working interactive demo / prototype
 * SIMULATION: High-fidelity sandbox for UI and DaVinci scopes validation
 * ARCHITECTURE_ONLY: Complete native code/driver specs ready for native toolchain compilation
 * NOT_IMPLEMENTED: Planned but not yet written
 * BLOCKED: Requires OS signing / hardware entitlements
 */
export enum FeatureStatus {
  IMPLEMENTED = 'IMPLEMENTED',
  IMPLEMENTED_UNTESTED = 'IMPLEMENTED / UNTESTED',
  PROTOTYPE = 'PROTOTYPE',
  SIMULATION = 'SIMULATION',
  ARCHITECTURE_ONLY = 'ARCHITECTURE_ONLY',
  NOT_IMPLEMENTED = 'NOT_IMPLEMENTED',
  PENDING = 'PENDING',
  BLOCKED = 'BLOCKED',
}

export interface FeatureMatrixItem {
  id: string;
  category: 'Virtual Display' | 'Capture & Pipeline' | 'Hardware Encoding' | 'Network & Protocol' | 'Hardware Decoding' | 'Client UI & Fullscreen' | 'Creative Suite Integration';
  feature: string;
  platform: Platform | 'ALL';
  targetPlatform?: Platform;
  status: FeatureStatus;
  technology: string;
  tested: boolean;
  notes: string;
}

export interface HostState {
  os: Platform.WINDOWS | Platform.MACOS;
  hostName: string;
  ipAddress: string;
  port: number;
  isServerRunning: boolean;
  virtualDisplayActive: boolean;
  displays: DisplayConfigPayload[];
  activeDisplayId: number;
  encoder: EncoderType;
  selectedCodec: VideoCodec;
  qualityPreset: QualityPreset;
  pairingPin: string;
  pinExpirySeconds: number;
  trustedClients: TrustedClient[];
}

export interface TrustedClient {
  id: string;
  name: string;
  platform: Platform.ANDROID | Platform.IOS_IPADOS;
  ip: string;
  pairedAt: string;
  lastConnected: string;
  status: 'ONLINE' | 'OFFLINE' | 'STREAMING';
}

export interface ClientState {
  platform: Platform.ANDROID | Platform.IOS_IPADOS;
  clientName: string;
  ipAddress: string;
  networkState: NetworkState;
  connectedHost: {
    name: string;
    ip: string;
    os: Platform.WINDOWS | Platform.MACOS;
  } | null;
  decoder: DecoderType;
  fullScreenActive: boolean;
  scalingMode: 'CONTAIN' | 'COVER' | 'NATIVE_100';
  orientation: 'LANDSCAPE' | 'PORTRAIT';
  activeSource: 'SIMULATION' | 'REAL_NETWORK';
  latencyMode: 'ULTRA_LOW' | 'SMOOTH';
}

export interface DaVinciScopeState {
  activeScope: 'RGB_PARADE' | 'WAVEFORM' | 'VECTORSCOPE' | 'HISTOGRAM' | 'ALL_QUAD';
  sourceImage: string;
  lift: { r: number; g: number; b: number; y: number };
  gamma: { r: number; g: number; b: number; y: number };
  gain: { r: number; g: number; b: number; y: number };
  saturation: number;
  contrast: number;
  colorTemperature: number; // Kelvin offset (-50 to +50)
  scopesOnDisplay2: boolean;
}

export interface CombinationMatrixItem {
  id: string;
  host: Platform.WINDOWS | Platform.MACOS;
  client: Platform.ANDROID | Platform.IOS_IPADOS;
  title: string;
  protocolStatus: FeatureStatus;
  virtualDisplayStatus: FeatureStatus;
  captureStatus: FeatureStatus;
  encodeDecodeStatus: FeatureStatus;
  overallStatus: FeatureStatus;
  minHostOS: string;
  minClientOS: string;
  transport: string;
  targetLatency: string;
  notes: string;
}

export interface DiagnosticItem {
  id: string;
  name: string;
  category: 'HOST_DRIVER' | 'CAPTURE' | 'ENCODER' | 'NETWORK' | 'DECODER' | 'CLIENT_SURFACE';
  status: 'PASS' | 'WARN' | 'FAIL' | 'PENDING';
  measuredValue: string;
  expectedValue: string;
  detail: string;
  isRealMetric: boolean;
}
