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

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_STOP_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbDevice::StopDevice() noexcept {
    PAGED_CODE();
    return STATUS_NOT_IMPLEMENTED;
}
