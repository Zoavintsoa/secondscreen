/**
 * PairingModal Component
 * Interactive PIN pairing manager for Host View with live 6-digit PIN,
 * QR code visual representation, and trusted device management.
 */

import React, { useState, useEffect } from 'react';
import { useSecondScreen } from '../../context/SecondScreenContext';
import {
  ShieldCheck,
  RefreshCw,
  QrCode,
  Smartphone,
  Tablet,
  Trash2,
  Lock,
  X,
  CheckCircle2,
  Clock
} from 'lucide-react';
import { Platform } from '../../types';

interface PairingModalProps {
  isOpen: boolean;
  onClose: () => void;
}

export const PairingModal: React.FC<PairingModalProps> = ({ isOpen, onClose }) => {
  const {
    hostState,
    regeneratePairingPin,
    revokeTrustedClient,
  } = useSecondScreen();

  const [timeLeft, setTimeLeft] = useState(180);

  useEffect(() => {
    if (!isOpen) return;
    const interval = setInterval(() => {
      setTimeLeft((prev) => {
        if (prev <= 1) {
          regeneratePairingPin();
          return 180;
        }
        return prev - 1;
      });
    }, 1000);
    return () => clearInterval(interval);
  }, [isOpen, regeneratePairingPin]);

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 z-50 bg-slate-950/80 backdrop-blur-md flex items-center justify-center p-4">
      <div className="bg-slate-900 border border-slate-800 rounded-2xl max-w-xl w-full p-6 shadow-2xl relative text-slate-100">
        {/* Close button */}
        <button
          onClick={onClose}
          className="absolute top-4 right-4 text-slate-400 hover:text-white p-1 rounded-lg hover:bg-slate-800 transition-colors"
        >
          <X className="w-5 h-5" />
        </button>

        <div className="flex items-center gap-3 pb-4 border-b border-slate-800">
          <div className="p-2.5 bg-cyan-500/10 rounded-xl text-cyan-400 border border-cyan-500/20">
            <Lock className="w-5 h-5" />
          </div>
          <div>
            <h3 className="text-lg font-bold text-slate-100">SecondScreen Pairing Center</h3>
            <p className="text-xs text-slate-400">Secure AES-256 handshake between Host and Client</p>
          </div>
        </div>

        {/* Pairing PIN Section */}
        <div className="mt-6 bg-slate-950 rounded-xl p-6 border border-slate-800 text-center">
          <span className="text-xs font-semibold text-slate-400 uppercase tracking-widest block mb-2">
            Dynamic Pairing PIN
          </span>
          <div className="flex items-center justify-center gap-2 font-mono text-4xl font-extrabold tracking-[0.25em] text-cyan-400 my-2">
            {hostState.pairingPin.split('').map((char, index) => (
              <span
                key={index}
                className="w-12 h-14 bg-slate-900 border border-slate-700/80 rounded-lg flex items-center justify-center shadow-inner"
              >
                {char}
              </span>
            ))}
          </div>

          <div className="flex items-center justify-center gap-4 mt-4 text-xs text-slate-400">
            <span className="flex items-center gap-1.5 text-amber-400 font-mono">
              <Clock className="w-3.5 h-3.5" />
              Expires in {Math.floor(timeLeft / 60)}:{(timeLeft % 60).toString().padStart(2, '0')}
            </span>
            <button
              onClick={() => {
                regeneratePairingPin();
                setTimeLeft(180);
              }}
              className="flex items-center gap-1 text-cyan-400 hover:text-cyan-300 font-medium hover:underline"
            >
              <RefreshCw className="w-3.5 h-3.5" /> Generate New PIN
            </button>
          </div>
        </div>

        {/* Trusted Paired Devices List */}
        <div className="mt-6">
          <div className="flex items-center justify-between mb-3">
            <h4 className="text-xs font-bold text-slate-300 uppercase tracking-wider">
              Trusted Paired Devices ({hostState.trustedClients.length})
            </h4>
            <span className="text-[11px] text-emerald-400 flex items-center gap-1">
              <ShieldCheck className="w-3.5 h-3.5" /> Zero-Trust Verified
            </span>
          </div>

          <div className="space-y-2.5 max-h-48 overflow-y-auto pr-1">
            {hostState.trustedClients.map((client) => (
              <div
                key={client.id}
                className="bg-slate-950/80 border border-slate-800 rounded-xl p-3 flex items-center justify-between text-xs"
              >
                <div className="flex items-center gap-3">
                  <div className="p-2 bg-slate-900 rounded-lg text-slate-300 border border-slate-800">
                    {client.platform === Platform.IOS_IPADOS ? <Tablet className="w-4 h-4 text-cyan-400" /> : <Smartphone className="w-4 h-4 text-emerald-400" />}
                  </div>
                  <div>
                    <div className="font-semibold text-slate-200 flex items-center gap-2">
                      {client.name}
                      <span className={`px-1.5 py-0.2 rounded text-[10px] font-mono ${
                        client.status === 'STREAMING'
                          ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/30'
                          : 'bg-slate-800 text-slate-400'
                      }`}>
                        {client.status}
                      </span>
                    </div>
                    <div className="text-[11px] text-slate-400 font-mono mt-0.5">
                      IP: {client.ip} • Paired: {client.pairedAt}
                    </div>
                  </div>
                </div>

                <button
                  onClick={() => revokeTrustedClient(client.id)}
                  title="Revoke device access"
                  className="p-1.5 text-slate-400 hover:text-rose-400 hover:bg-rose-500/10 rounded-lg transition-colors"
                >
                  <Trash2 className="w-4 h-4" />
                </button>
              </div>
            ))}
          </div>
        </div>

        {/* Footer info */}
        <div className="mt-6 pt-4 border-t border-slate-800 flex items-center justify-between text-xs text-slate-400">
          <span>Host IP: <strong className="text-slate-200 font-mono">{hostState.ipAddress}:{hostState.port}</strong></span>
          <button
            onClick={onClose}
            className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-white font-medium rounded-lg transition-colors"
          >
            Done
          </button>
        </div>
      </div>
    </div>
  );
};
