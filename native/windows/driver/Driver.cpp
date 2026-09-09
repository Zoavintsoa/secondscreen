// SecondScreen IddCx Virtual Display Driver - Driver Initialization
// Copyright (c) 2026 SecondScreen Project. All rights reserved.

#include "Driver.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    WDF_DRIVER_CONFIG Config;
    WDF_DRIVER_CONFIG_INIT(&Config, SecondScreenDeviceAdd);

    NTSTATUS Status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &Config, WDF_NO_HANDLE);
    return Status;
}

NTSTATUS SecondScreenDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit) {
    UNREFERENCED_PARAMETER(Driver);

    IDDCX_CLIENT_CONFIG IddCxConfig;
    IDDCX_CLIENT_CONFIG_INIT(&IddCxConfig);
    IddCxConfig.EvtIddCxAdapterInitFinished = SecondScreenAdapterInitFinished;
    IddCxConfig.EvtIddCxAdapterCommitModes = SecondScreenCommitModes;
    IddCxConfig.EvtIddCxParseMonitorDescription = SecondScreenParseMonitorDescription;
    IddCxConfig.EvtIddCxMonitorGetDefaultDescriptionModes = SecondScreenGetDefaultModes;
    IddCxConfig.EvtIddCxMonitorQueryTargetModes = SecondScreenQueryTargetModes;
    IddCxConfig.EvtIddCxMonitorAssignSwapChain = SecondScreenAssignSwapChain;
    IddCxConfig.EvtIddCxMonitorUnassignSwapChain = SecondScreenUnassignSwapChain;

    NTSTATUS Status = IddCxDeviceInitConfig(DeviceInit, &IddCxConfig);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    WDF_PNPPOWER_EVENT_CALLBACKS PnpCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpCallbacks);
    PnpCallbacks.EvtDeviceD0Entry = SecondScreenDeviceD0Entry;
    PnpCallbacks.EvtDeviceD0Exit = SecondScreenDeviceD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpCallbacks);

    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, IndirectDeviceContext);

    WDFDEVICE Device;
    Status = WdfDeviceCreate(&DeviceInit, &Attributes, &Device);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    auto* pContext = WdfObjectGetTypedContext(Device, IndirectDeviceContext);
    pContext->WdfDevice = Device;

    pContext->TerminateWorker = FALSE;
    return STATUS_SUCCESS;
}

NTSTATUS SecondScreenDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState) {
    auto* pContext = WdfObjectGetTypedContext(Device, IndirectDeviceContext);
    if (pContext->IddAdapter != nullptr) {
        return STATUS_SUCCESS;
    }

    IDDCX_ADAPTER_CAPS Caps = {};
    Caps.Size = sizeof(Caps);
    Caps.MaxMonitorsSupported = 1;

    WDF_OBJECT_ATTRIBUTES AdapterAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&AdapterAttributes, IndirectAdapterContext);

    IDARG_IN_ADAPTER_INIT AdapterInit = {};
    AdapterInit.WdfDevice = Device;
    AdapterInit.pCaps = &Caps;
    AdapterInit.ObjectAttributes = &AdapterAttributes;

    IDARG_OUT_ADAPTER_INIT AdapterOut = {};
    NTSTATUS Status = IddCxAdapterInitAsync(&AdapterInit, &AdapterOut);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    pContext->IddAdapter = AdapterOut.AdapterObject;
    auto* adapterContext = WdfObjectGetTypedContext(AdapterOut.AdapterObject, IndirectAdapterContext);
    adapterContext->DeviceContext = pContext;
    UNREFERENCED_PARAMETER(PreviousState);
    return STATUS_SUCCESS;
}

NTSTATUS SecondScreenDeviceD0Exit(WDFDEVICE Device, WDF_POWER_DEVICE_STATE TargetState) {
    UNREFERENCED_PARAMETER(TargetState);
    auto* pContext = WdfObjectGetTypedContext(Device, IndirectDeviceContext);
    if (pContext->IddMonitor != nullptr) {
        IddCxMonitorDeparture(pContext->IddMonitor);
        pContext->IddMonitor = nullptr;
    }
    if (pContext->WorkerThread) {
        InterlockedExchange(&pContext->TerminateWorker, TRUE);
        SetEvent(pContext->SwapChainEvent);
        WaitForSingleObject(pContext->WorkerThread, 2000);
        CloseHandle(pContext->WorkerThread);
        pContext->WorkerThread = nullptr;
    }
    pContext->IddSwapChain = nullptr;
    pContext->SwapChainEvent = nullptr;
    pContext->D3DContext.Reset();
    pContext->D3DDevice.Reset();
    return STATUS_SUCCESS;
}
