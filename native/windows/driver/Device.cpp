// SecondScreen IddCx Virtual Display Driver - Monitor, Modes & Swapchain Handler
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "Driver.h"

namespace {
constexpr UINT kModeCount = 3;

struct ModeDefinition {
    UINT width;
    UINT height;
    UINT refreshRate;
};

constexpr ModeDefinition kModes[kModeCount] = {
    { 1280, 720, 60 },
    { 1920, 1080, 60 },
    { 2560, 1440, 60 },
};

void PopulateMonitorMode(IDDCX_MONITOR_MODE* mode, const ModeDefinition& definition) {
    mode->Size = sizeof(*mode);
    mode->Origin = IDDCX_MODE_ORIGIN_DRIVER;
    mode->MonitorVideoSignalInfo.hSyncFreq.Numerator = definition.refreshRate * definition.height;
    mode->MonitorVideoSignalInfo.hSyncFreq.Denominator = 1;
    mode->MonitorVideoSignalInfo.vSyncFreq.Numerator = definition.refreshRate;
    mode->MonitorVideoSignalInfo.vSyncFreq.Denominator = 1;
    mode->MonitorVideoSignalInfo.activeSize.cx = definition.width;
    mode->MonitorVideoSignalInfo.activeSize.cy = definition.height;
    mode->MonitorVideoSignalInfo.totalSize.cx = definition.width;
    mode->MonitorVideoSignalInfo.totalSize.cy = definition.height;
    mode->MonitorVideoSignalInfo.videoStandard = D3DKMDT_VSS_OTHER;
}
}

NTSTATUS SecondScreenAdapterInitFinished(IDDCX_ADAPTER AdapterObject, const IDARG_IN_ADAPTER_INIT_FINISHED* pInArgs) {
    if (!NT_SUCCESS(pInArgs->AdapterInitStatus)) {
        return pInArgs->AdapterInitStatus;
    }

    auto* adapterContext = WdfObjectGetTypedContext(AdapterObject, IndirectAdapterContext);
    auto* pContext = adapterContext->DeviceContext;
    if (pContext == nullptr) {
        return STATUS_DEVICE_NOT_READY;
    }

    // Create Virtual Monitor (Display 2)
    IDDCX_MONITOR_INFO MonitorInfo = {};
    MonitorInfo.Size = sizeof(MonitorInfo);
    MonitorInfo.MonitorType = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED;
    MonitorInfo.ConnectorIndex = 0;
    
    MonitorInfo.MonitorDescription.Size = sizeof(MonitorInfo.MonitorDescription);
    MonitorInfo.MonitorDescription.Type = IDDCX_MONITOR_DESCRIPTION_TYPE_EDID;
    MonitorInfo.MonitorDescription.DataSize = sizeof(s_SecondScreenEdidBlock);
    MonitorInfo.MonitorDescription.pData = const_cast<BYTE*>(s_SecondScreenEdidBlock);

    WDF_OBJECT_ATTRIBUTES MonitorAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&MonitorAttributes, IndirectMonitorContext);

    IDARG_IN_MONITORCREATE MonitorCreate = {};
    MonitorCreate.ObjectAttributes = &MonitorAttributes;
    MonitorCreate.pMonitorInfo = &MonitorInfo;

    IDARG_OUT_MONITORCREATE MonitorCreateOut = {};
    NTSTATUS Status = IddCxMonitorCreate(AdapterObject, &MonitorCreate, &MonitorCreateOut);
    if (NT_SUCCESS(Status)) {
        pContext->IddMonitor = MonitorCreateOut.MonitorObject;
        auto* monitorContext = WdfObjectGetTypedContext(pContext->IddMonitor, IndirectMonitorContext);
        monitorContext->DeviceContext = pContext;

        IDARG_OUT_MONITORARRIVAL ArrivalOut = {};
        Status = IddCxMonitorArrival(pContext->IddMonitor, &ArrivalOut);
    }

    return Status;
}

NTSTATUS SecondScreenCommitModes(IDDCX_ADAPTER AdapterObject, const IDARG_IN_COMMITMODES* pInArgs) {
    UNREFERENCED_PARAMETER(AdapterObject);
    UNREFERENCED_PARAMETER(pInArgs);
    return STATUS_SUCCESS;
}

NTSTATUS SecondScreenParseMonitorDescription(const IDARG_IN_PARSEMONITORDESCRIPTION* pInArgs, IDARG_OUT_PARSEMONITORDESCRIPTION* pOutArgs) {
    pOutArgs->MonitorModeBufferOutputCount = kModeCount;

    if (pInArgs->MonitorModeBufferInputCount < pOutArgs->MonitorModeBufferOutputCount) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (UINT i = 0; i < kModeCount; ++i) {
        PopulateMonitorMode(&pInArgs->pMonitorModes[i], kModes[i]);
    }

    pOutArgs->PreferredMonitorModeIdx = 1;
    return STATUS_SUCCESS;
}

NTSTATUS SecondScreenGetDefaultModes(IDDCX_MONITOR MonitorObject, const IDARG_IN_GETDEFAULTDESCRIPTIONMODES* pInArgs, IDARG_OUT_GETDEFAULTDESCRIPTIONMODES* pOutArgs) {
    UNREFERENCED_PARAMETER(MonitorObject);
    UNREFERENCED_PARAMETER(pInArgs);
    pOutArgs->DefaultMonitorModeBufferOutputCount = 0;
    return STATUS_SUCCESS;
}

NTSTATUS SecondScreenQueryTargetModes(IDDCX_MONITOR MonitorObject, const IDARG_IN_QUERYTARGETMODES* pInArgs, IDARG_OUT_QUERYTARGETMODES* pOutArgs) {
    UNREFERENCED_PARAMETER(MonitorObject);
    pOutArgs->TargetModeBufferOutputCount = kModeCount;

    if (pInArgs->TargetModeBufferInputCount < kModeCount) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (UINT i = 0; i < kModeCount; ++i) {
        pInArgs->pTargetModes[i].Size = sizeof(IDDCX_TARGET_MODE);
        pInArgs->pTargetModes[i].TargetVideoSignalInfo.targetId = i;
        pInArgs->pTargetModes[i].TargetVideoSignalInfo.activeSize.cx = kModes[i].width;
        pInArgs->pTargetModes[i].TargetVideoSignalInfo.activeSize.cy = kModes[i].height;
        pInArgs->pTargetModes[i].TargetVideoSignalInfo.totalSize.cx = kModes[i].width;
        pInArgs->pTargetModes[i].TargetVideoSignalInfo.totalSize.cy = kModes[i].height;
        pInArgs->pTargetModes[i].TargetVideoSignalInfo.vSyncFreq.Numerator = kModes[i].refreshRate;
        pInArgs->pTargetModes[i].TargetVideoSignalInfo.vSyncFreq.Denominator = 1;
    }

    return STATUS_SUCCESS;
}

NTSTATUS SecondScreenAssignSwapChain(IDDCX_MONITOR MonitorObject, const IDARG_IN_SETSWAPCHAIN* pInArgs) {
    auto* monitorContext = WdfObjectGetTypedContext(MonitorObject, IndirectMonitorContext);
    auto* pContext = monitorContext->DeviceContext;
    if (pContext == nullptr) {
        return STATUS_DEVICE_NOT_READY;
    }
    pContext->IddSwapChain = pInArgs->hSwapChain;
    pContext->SwapChainEvent = pInArgs->hNextSurfaceAvailable;
    InterlockedExchange(&pContext->TerminateWorker, FALSE);

    IDARG_IN_SWAPCHAINSETDEVICE SetDevice = {};
    NTSTATUS Status = CreateRenderDevice(pInArgs->pRenderAdapterLuid, pContext);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    SetDevice.hDevice = pContext->D3DDevice.Get();

    Status = IddCxSwapChainSetDevice(pContext->IddSwapChain, &SetDevice);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    pContext->WorkerThread = CreateThread(nullptr, 0, SwapChainWorkerThread, pContext, 0, nullptr);
    return STATUS_SUCCESS;
}

NTSTATUS SecondScreenUnassignSwapChain(IDDCX_MONITOR MonitorObject) {
    auto* monitorContext = WdfObjectGetTypedContext(MonitorObject, IndirectMonitorContext);
    auto* pContext = monitorContext->DeviceContext;
    if (pContext == nullptr) {
        return STATUS_SUCCESS;
    }
    if (pContext->WorkerThread) {
        InterlockedExchange(&pContext->TerminateWorker, TRUE);
        SetEvent(pContext->SwapChainEvent);
        WaitForSingleObject(pContext->WorkerThread, 2000);
        CloseHandle(pContext->WorkerThread);
        pContext->WorkerThread = nullptr;
    }
    pContext->SwapChainEvent = nullptr;
    pContext->IddSwapChain = nullptr;
    pContext->D3DContext.Reset();
    pContext->D3DDevice.Reset();
    return STATUS_SUCCESS;
}

DWORD CALLBACK SwapChainWorkerThread(LPVOID lpParam) {
    auto* pContext = reinterpret_cast<IndirectDeviceContext*>(lpParam);

    while (InterlockedCompareExchange(&pContext->TerminateWorker, FALSE, FALSE) == FALSE) {
        IDARG_OUT_SWAPCHAINRELEASEANDACQUIREBUFFER OutAcquire = {};
        HRESULT Status = IddCxSwapChainReleaseAndAcquireBuffer(pContext->IddSwapChain, &OutAcquire);

        if (SUCCEEDED(Status)) {
            IddCxSwapChainFinishedProcessingFrame(pContext->IddSwapChain);
        } else if (Status != E_PENDING) {
            break;
        }

        WaitForSingleObject(pContext->SwapChainEvent, 16);
    }

    return 0;
}

NTSTATUS CreateRenderDevice(const LUID* renderAdapterLuid, IndirectDeviceContext* deviceContext) {
    if (renderAdapterLuid == nullptr || deviceContext == nullptr) {
        return STATUS_INVALID_PARAMETER;
    }

    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return HRESULT_TO_NT(hr);
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    hr = factory->EnumAdapterByLuid(*renderAdapterLuid, IID_PPV_ARGS(&adapter));
    if (FAILED(hr)) {
        return HRESULT_TO_NT(hr);
    }

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(
        adapter.Get(),
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        flags,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &deviceContext->D3DDevice,
        &featureLevel,
        &deviceContext->D3DContext);

    return SUCCEEDED(hr) ? STATUS_SUCCESS : HRESULT_TO_NT(hr);
}
