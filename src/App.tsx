/**
 * SecondScreen Application Root
 * Multiplatform Virtual Display System (Windows/macOS -> Android/iOS)
 */

import React from 'react';
import { SecondScreenProvider, useSecondScreen } from './context/SecondScreenContext';
import { HeaderNav } from './components/HeaderNav';
import { DualScreenSimulator } from './components/LiveDualScreen/DualScreenSimulator';
import { HostDashboard } from './components/HostView/HostDashboard';
import { ClientScreen } from './components/ClientOnlyView/ClientScreen';
import { DiagnosticsView } from './components/Diagnostics/DiagnosticsView';
import { NativeArchitectureView } from './components/ArchitectureDocs/NativeArchitectureView';

const MainContent: React.FC = () => {
  const { activeTab } = useSecondScreen();

  switch (activeTab) {
    case 'DUAL_SCREEN_LAB':
      return <DualScreenSimulator />;
    case 'HOST_VIEW':
      return <HostDashboard />;
    case 'CLIENT_VIEW':
      return <ClientScreen />;
    case 'DIAGNOSTICS':
      return <DiagnosticsView />;
    case 'ARCHITECTURE':
      return <NativeArchitectureView />;
    default:
      return <DualScreenSimulator />;
  }
};

export default function App() {
  return (
    <SecondScreenProvider>
      <div className="flex flex-col min-h-screen bg-slate-950 text-slate-100 font-sans pb-16 lg:pb-0">
        <HeaderNav />
        <main className="flex-1 flex flex-col overflow-hidden">
          <MainContent />
        </main>
      </div>
    </SecondScreenProvider>
  );
}

