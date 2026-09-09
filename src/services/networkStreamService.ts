/**
 * Network Stream Service & Protocol State Machine
 * Handles protocol serialization and browser display capture for UI development.
 * Browser capture is not a virtual display, GPU pipeline, or network-stream validation.
 */

import {
  MessageType,
  NetworkState,
  ProtocolPacket,
  TelemetryPayload,
  InputEventPayload,
  PROTOCOL_VERSION,
  VideoCodec
} from '../types/protocol';

export type PacketListener = (packet: ProtocolPacket) => void;
export type TelemetryListener = (telemetry: TelemetryPayload) => void;

class NetworkStreamService {
  private state: NetworkState = NetworkState.DISCONNECTED;
  private listeners: Map<string, Set<PacketListener>> = new Map();
  private telemetryListeners: Set<TelemetryListener> = new Set();
  private sequence = 0;
  private sessionId: string | null = null;
  private rttTimer: number | null = null;
  private currentStream: MediaStream | null = null;
  private isCapturingRealScreen = false;

  public getState(): NetworkState {
    return this.state;
  }

  public setState(newState: NetworkState): void {
    this.state = newState;
    this.emit('STATE_CHANGED', {
      version: PROTOCOL_VERSION,
      type: MessageType.TELEMETRY,
      timestamp: Date.now(),
      sequence: ++this.sequence,
      payload: { state: newState }
    });
  }

  public on(event: string, callback: PacketListener): () => void {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, new Set());
    }
    this.listeners.get(event)!.add(callback);
    return () => {
      this.listeners.get(event)?.delete(callback);
    };
  }

  public onTelemetry(callback: TelemetryListener): () => void {
    this.telemetryListeners.add(callback);
    return () => {
      this.telemetryListeners.delete(callback);
    };
  }

  private emit(event: string, packet: ProtocolPacket): void {
    this.listeners.get(event)?.forEach((cb) => cb(packet));
    this.listeners.get('*')?.forEach((cb) => cb(packet));
  }

  /**
   * Request actual real screen capture from host desktop browser if requested by user
   */
  public async requestRealScreenCapture(): Promise<MediaStream | null> {
    try {
      if (navigator.mediaDevices && navigator.mediaDevices.getDisplayMedia) {
        const stream = await navigator.mediaDevices.getDisplayMedia({
          video: {
            frameRate: { ideal: 60, max: 120 },
            displaySurface: 'monitor',
          },
          audio: false,
        });
        this.currentStream = stream;
        this.isCapturingRealScreen = true;
        this.setState(NetworkState.STREAMING);
        return stream;
      }
    } catch (err) {
      console.warn('[SecondScreen] Real screen capture canceled or denied:', err);
    }
    return null;
  }

  public stopRealScreenCapture(): void {
    if (this.currentStream) {
      this.currentStream.getTracks().forEach((track) => track.stop());
      this.currentStream = null;
    }
    this.isCapturingRealScreen = false;
  }

  public getRealStream(): MediaStream | null {
    return this.currentStream;
  }

  public isRealCaptureActive(): boolean {
    return this.isCapturingRealScreen;
  }

  /**
   * Send Client Input Event (Touch / Mouse / Stylus) back to Host
   */
  public sendInputEvent(input: Omit<InputEventPayload, 'timestamp'>): void {
    const packet: ProtocolPacket<InputEventPayload> = {
      version: PROTOCOL_VERSION,
      type: MessageType.INPUT_EVENT,
      timestamp: Date.now(),
      sequence: ++this.sequence,
      sessionId: this.sessionId || undefined,
      payload: {
        ...input,
        timestamp: Date.now(),
      },
    };
    this.emit('INPUT_EVENT', packet);
  }

  /**
   * Validate 6-digit PIN and establish authenticated session
   */
  public attemptPairing(pin: string, expectedPin: string, clientName: string): boolean {
    if (pin === expectedPin) {
      this.sessionId = 'sess_' + Math.random().toString(36).substring(2, 9);
      this.setState(NetworkState.CONNECTING);
      setTimeout(() => {
        this.setState(NetworkState.STREAMING);
      }, 400);
      return true;
    }
    return false;
  }

  public disconnect(): void {
    this.stopRealScreenCapture();
    this.sessionId = null;
    this.setState(NetworkState.DISCONNECTED);
  }

}

export const networkStreamService = new NetworkStreamService();
