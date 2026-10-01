#include "device.hpp"
#include "xenbus.hpp"

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_START_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbDevice::StartDevice(
        IN_PDXGK_START_INFO DxgkStartInfo,
        IN_PDXGKRNL_INTERFACE DxgkInterface,
        OUT_PULONG NumberOfVideoPresentSources,
        OUT_PULONG NumberOfChildren) noexcept {
    NTSTATUS Status;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DxgkStartInfo);
    UNREFERENCED_PARAMETER(NumberOfVideoPresentSources);
    UNREFERENCED_PARAMETER(NumberOfChildren);

    if (_started) {
        return STATUS_UNSUCCESSFUL;
    }

    if (DxgkInterface->Version < DXGKDDI_INTERFACE_VERSION || DxgkInterface->Size < sizeof(*DxgkInterface)) {
        return STATUS_NOT_SUPPORTED;
    }

    _dxgk = *DxgkInterface;

    Status = DebugInterface::Query(_debug, _pdo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = EvtchnInterface::Query(_evtchn, _pdo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = StoreInterface::Query(_store, _pdo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = SuspendInterface::Query(_suspend, _pdo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    _started = true;
    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_STOP_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbDevice::StopDevice() noexcept {
    PAGED_CODE();
    return STATUS_NOT_IMPLEMENTED;
}
