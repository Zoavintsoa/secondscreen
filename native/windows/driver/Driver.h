// SecondScreen IddCx Virtual Display Driver (UMDF 2.0)
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#pragma once

#define NOMINMAX
#include <windows.h>
#include <bugcodes.h>
#include <wdf.h>
#include <iddcx.h>
#include <d3d11_2.h>
#include <dxgi1_3.h>
#include <wrl/client.h>

#define SECONDSCREEN_MONITOR_EDID_BLOCK_COUNT 1

static const BYTE s_SecondScreenEdidBlock[128] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x4D, 0x29, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x1E, 0x20, 0x01, 0x04, 0xA5, 0x3C, 0x22, 0x78, 0x3B, 0xEE, 0x95, 0xA3, 0x54, 0x4C, 0x99, 0x26,
    0x0F, 0x50, 0x54, 0xA5, 0x4B, 0x00, 0x71, 0x4F, 0x81, 0x80, 0xA9, 0xC0, 0xB3, 0x00, 0xD1, 0xC0,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
    0x45, 0x00, 0x56, 0x5E, 0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x38, 0x4B, 0x1E,
    0x53, 0x0F, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x53,
    0x65, 0x63, 0x6F, 0x6E, 0x64, 0x53, 0x63, 0x72, 0x65, 0x65, 0x6E, 0x0A, 0x00, 0x00, 0x00, 0xFF,
    0x00, 0x53, 0x45, 0x43, 0x4F, 0x4E, 0x44, 0x30, 0x30, 0x31, 0x0A, 0x20, 0x20, 0x20, 0x00, 0x11
};

struct IndirectDeviceContext {
    WDFDEVICE WdfDevice;
    IDDCX_ADAPTER IddAdapter;
    IDDCX_MONITOR IddMonitor;
    IDDCX_SWAPCHAIN IddSwapChain;
    HANDLE SwapChainEvent;
    HANDLE WorkerThread;
    volatile LONG TerminateWorker;
    Microsoft::WRL::ComPtr<ID3D11Device> D3DDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> D3DContext;
};
WDF_DECLARE_CONTEXT_TYPE(IndirectDeviceContext);

struct IndirectAdapterContext {
    IndirectDeviceContext* DeviceContext;
};
WDF_DECLARE_CONTEXT_TYPE(IndirectAdapterContext);

struct IndirectMonitorContext {
    IndirectDeviceContext* DeviceContext;
};
WDF_DECLARE_CONTEXT_TYPE(IndirectMonitorContext);

extern "C" DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD SecondScreenDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY SecondScreenDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT SecondScreenDeviceD0Exit;

EVT_IDD_CX_ADAPTER_INIT_FINISHED SecondScreenAdapterInitFinished;
EVT_IDD_CX_ADAPTER_COMMIT_MODES SecondScreenCommitModes;
EVT_IDD_CX_PARSE_MONITOR_DESCRIPTION SecondScreenParseMonitorDescription;
EVT_IDD_CX_MONITOR_GET_DEFAULT_DESCRIPTION_MODES SecondScreenGetDefaultModes;
EVT_IDD_CX_MONITOR_QUERY_TARGET_MODES SecondScreenQueryTargetModes;
EVT_IDD_CX_MONITOR_ASSIGN_SWAPCHAIN SecondScreenAssignSwapChain;
EVT_IDD_CX_MONITOR_UNASSIGN_SWAPCHAIN SecondScreenUnassignSwapChain;

DWORD CALLBACK SwapChainWorkerThread(LPVOID lpParam);
NTSTATUS CreateRenderDevice(const LUID* renderAdapterLuid, IndirectDeviceContext* deviceContext);
