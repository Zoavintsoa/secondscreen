/**
 * Protocol v1 - SecondScreen Common Wire Protocol Definition
 * Cross-platform packet specification for Windows/macOS Host <-> Android/iOS Client
 */

export const PROTOCOL_VERSION = 1;

export enum MessageType {
  HELLO = 'HELLO',
  DISCOVER = 'DISCOVER',
  PAIR_REQUEST = 'PAIR_REQUEST',
  PAIR_RESPONSE = 'PAIR_RESPONSE',
  DISPLAY_CONFIG = 'DISPLAY_CONFIG',
  STREAM_START = 'STREAM_START',
  STREAM_STOP = 'STREAM_STOP',
  FRAME = 'FRAME',
  PING = 'PING',
  PONG = 'PONG',
  TELEMETRY = 'TELEMETRY',
  INPUT_EVENT = 'INPUT_EVENT',
  DISCONNECT = 'DISCONNECT',
}

export enum NetworkState {
  DISCONNECTED = 'DISCONNECTED',
  DISCOVERING = 'DISCOVERING',
  PAIRING = 'PAIRING',
  CONNECTING = 'CONNECTING',
  STREAMING = 'STREAMING',
  PAUSED = 'PAUSED',
  ERROR = 'ERROR',
}

export enum DisplayMode {
  EXTEND = 'EXTEND',
  DUPLICATE = 'DUPLICATE',
  SECOND_SCREEN_ONLY = 'SECOND_SCREEN_ONLY',
}

export enum VideoCodec {
  H264 = 'H.264 (AVC)',
  HEVC = 'HEVC (H.265)',
  AV1 = 'AV1',
}

export enum EncoderType {
  NVENC = 'NVIDIA NVENC',
  AMF = 'AMD AMF',
  QUICKSYNC = 'Intel Quick Sync',
  VIDEOTOOLBOX = 'Apple VideoToolbox',
  SOFTWARE = 'Software (libx264 fallback)',
}

export enum DecoderType {
  MEDIACODEC = 'Android MediaCodec',
  VIDEOTOOLBOX = 'iOS VideoToolbox',
  WEB_CODECS = 'WebCodecs / Canvas2D',
  SOFTWARE = 'Software Decoder',
}

export enum QualityPreset {
  LOW_LATENCY = 'LOW_LATENCY', // 1080p @ 60fps, 15 Mbps
  BALANCED = 'BALANCED',       // 1440p @ 60fps, 25 Mbps
  QUALITY = 'QUALITY',         // 1600p/4K @ 60fps, 40 Mbps
}

export interface ProtocolPacket<T = unknown> {
  version: number;
  type: MessageType;
  timestamp: number;
  sequence: number;
  sessionId?: string;
  payload: T;
}

export interface HelloPayload {
  deviceName: string;
  platform: 'WINDOWS' | 'MACOS' | 'ANDROID' | 'IOS_IPADOS';
  clientVersion: string;
  supportedCodecs: VideoCodec[];
  capabilities: {
    hwDecoding: boolean;
    touchInput: boolean;
    stylusInput: boolean;
    maxResolution: { width: number; height: number };
    maxFps: number;
  };
}

export interface PairRequestPayload {
  pin: string;
  clientId: string;
  clientName: string;
  publicKey?: string;
}

export interface PairResponsePayload {
  success: boolean;
  sessionId: string;
  token: string;
  hostName: string;
  error?: string;
}

export interface DisplayConfigPayload {
  displayId: number;
  displayName: string;
  width: number;
  height: number;
  refreshRate: number; // 60, 90, 120
  orientation: 'PORTRAIT' | 'LANDSCAPE';
  mode: DisplayMode;
  colorSpace: 'sRGB' | 'Rec.709' | 'DCI-P3';
  dpiScale: number;
  isVirtual: boolean;
}

export interface StreamStartPayload {
  codec: VideoCodec;
  bitrateKbps: number;
  fps: number;
  keyFrameInterval: number;
  width: number;
  height: number;
  audioEnabled: boolean;
  transport: 'UDP_RTP' | 'TCP' | 'WEBSOCKET' | 'WEBRTC';
}

export interface TelemetryPayload {
  fps: number;
  bitrateMbps: number;
  rttMs: number;
  decodeLatencyMs: number;
  renderLatencyMs: number;
  totalLatencyMs: number;
  packetLossPercent: number;
  frameDrops: number;
  jitterMs: number;
  measured: boolean; // true = measured from socket, false = estimated
}

export interface InputEventPayload {
  type: 'POINTER_DOWN' | 'POINTER_MOVE' | 'POINTER_UP' | 'WHEEL' | 'KEY_DOWN' | 'KEY_UP' | 'STYLUS';
  x: number; // Normalized 0.0 - 1.0
  y: number; // Normalized 0.0 - 1.0
  button?: number;
  pressure?: number;
  key?: string;
  timestamp: number;
}
