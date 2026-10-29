#include "xenvfb.hpp"
#include <xenklib/allocator.hpp>
#include "device.hpp"

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_ADD_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
    VfbAddDevice(IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject, OUT_PPVOID MiniportDeviceContext) {
    PAGED_CODE();

    if (!MiniportDeviceContext) {
        return STATUS_INVALID_PARAMETER;
    }
    *MiniportDeviceContext = nullptr;

    auto device = MakeUniqueVfb<VfbDevice>(PhysicalDeviceObject);
    if (!device) {
        return STATUS_NO_MEMORY;
    }

    *MiniportDeviceContext = static_cast<PVOID>(device.release());
    return STATUS_SUCCESS;
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_START_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbStartDevice(
        IN_CONST_PVOID MiniportDeviceContext,
        IN_PDXGK_START_INFO DxgkStartInfo,
        IN_PDXGKRNL_INTERFACE DxgkInterface,
        OUT_PULONG NumberOfVideoPresentSources,
        OUT_PULONG NumberOfChildren) {
    PAGED_CODE();

    if (!MiniportDeviceContext) {
        return STATUS_INVALID_PARAMETER;
    }

    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->StartDevice(DxgkStartInfo, DxgkInterface, NumberOfVideoPresentSources, NumberOfChildren);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_STOP_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbStopDevice(IN_CONST_PVOID MiniportDeviceContext) {
    PAGED_CODE();

    if (!MiniportDeviceContext) {
        return STATUS_INVALID_PARAMETER;
    }

    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->StopDevice();
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_REMOVE_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbRemoveDevice(IN_CONST_PVOID MiniportDeviceContext) {
    PAGED_CODE();

    if (!MiniportDeviceContext) {
        return STATUS_INVALID_PARAMETER;
    }

    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    xenklib::TaggedPoolDeleter<VfbDevice, XenvfbTag>{}(device);

    return STATUS_SUCCESS;
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_DISPATCH_IO_REQUEST)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbDispatchIoRequest(
        IN_CONST_PVOID MiniportDeviceContext,
        IN_ULONG VidPnSourceId,
        IN_PVIDEO_REQUEST_PACKET VideoRequestPacket) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->DispatchIoRequest(VidPnSourceId, VideoRequestPacket);
}

_Check_return_ _Function_class_DXGK_(DXGKDDI_INTERRUPT_ROUTINE)
    _IRQL_requires_DXGK_(HIGH_LEVEL) // HIGH_LEVEL is best approximation of DIRQL
    BOOLEAN VfbInterruptRoutine(IN_CONST_PVOID MiniportDeviceContext, IN_ULONG MessageNumber) {
    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->InterruptRoutine(MessageNumber);
}

_Function_class_DXGK_(DXGKDDI_DPC_ROUTINE) _IRQL_requires_DXGK_(DISPATCH_LEVEL) VOID
    VfbDpcRoutine(IN_CONST_PVOID MiniportDeviceContext) {
    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    device->DpcRoutine();
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERY_CHILD_RELATIONS)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbQueryChildRelations(
        IN_CONST_PVOID MiniportDeviceContext,
        _Inout_updates_bytes_(ChildRelationsSize) PDXGK_CHILD_DESCRIPTOR ChildRelations,
        _In_ ULONG ChildRelationsSize) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->QueryChildRelations(ChildRelations, ChildRelationsSize);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERY_CHILD_STATUS)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbQueryChildStatus(
        IN_CONST_PVOID MiniportDeviceContext,
        INOUT_PDXGK_CHILD_STATUS ChildStatus,
        IN_BOOLEAN NonDestructiveOnly) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->QueryChildStatus(ChildStatus, NonDestructiveOnly);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERY_DEVICE_DESCRIPTOR)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbQueryDeviceDescriptor(
        IN_CONST_PVOID MiniportDeviceContext,
        IN_ULONG ChildUid,
        INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->QueryDeviceDescriptor(ChildUid, DeviceDescriptor);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_SET_POWER_STATE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbSetPowerState(
        IN_CONST_PVOID MiniportDeviceContext,
        IN_ULONG DeviceUid,
        IN_DEVICE_POWER_STATE DevicePowerState,
        IN_POWER_ACTION ActionType) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->SetPowerState(DeviceUid, DevicePowerState, ActionType);
}

_Function_class_DXGK_(DXGKDDI_RESET_DEVICE) VOID VfbResetDevice(IN_CONST_PVOID MiniportDeviceContext) {
    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    device->ResetDevice();
}

EXTERN_C DECLSPEC_PAGED_CODE _Function_class_DXGK_(DXGKDDI_UNLOAD) _IRQL_requires_DXGK_(PASSIVE_LEVEL) VOID
    VfbUnload(VOID) {
    PAGED_CODE();
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERYADAPTERINFO)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    VfbQueryAdapterInfo(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_QUERYADAPTERINFO pQueryAdapterInfo) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->QueryAdapterInfo(pQueryAdapterInfo);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_SETPOINTERPOSITION)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    VfbSetPointerPosition(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_SETPOINTERPOSITION pSetPointerPosition) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->SetPointerPosition(pSetPointerPosition);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_SETPOINTERSHAPE)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    VfbSetPointerShape(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_SETPOINTERSHAPE pSetPointerShape) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->SetPointerShape(pSetPointerShape);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_ISSUPPORTEDVIDPN)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    VfbIsSupportedVidPn(IN_CONST_HANDLE hAdapter, INOUT_PDXGKARG_ISSUPPORTEDVIDPN pIsSupportedVidPn) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->IsSupportedVidPn(pIsSupportedVidPn);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_RECOMMENDFUNCTIONALVIDPN)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY VfbRecommendFunctionalVidPn(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_RECOMMENDFUNCTIONALVIDPN_CONST pRecommendFunctionalVidPn) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->RecommendFunctionalVidPn(pRecommendFunctionalVidPn);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_ENUMVIDPNCOFUNCMODALITY)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY VfbEnumVidPnCofuncModality(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->EnumVidPnCofuncModality(pEnumCofuncModality);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_SETVIDPNSOURCEVISIBILITY)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY VfbSetVidPnSourceVisibility(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_SETVIDPNSOURCEVISIBILITY pSetVidPnSourceVisibility) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->SetVidPnSourceVisibility(pSetVidPnSourceVisibility);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_COMMITVIDPN)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    VfbCommitVidPn(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_COMMITVIDPN_CONST pCommitVidPn) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->CommitVidPn(pCommitVidPn);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY VfbUpdateActiveVidPnPresentPath(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_UPDATEACTIVEVIDPNPRESENTPATH_CONST pUpdateActiveVidPnPresentPath) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->UpdateActiveVidPnPresentPath(pUpdateActiveVidPnPresentPath);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_RECOMMENDMONITORMODES)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY VfbRecommendMonitorModes(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_RECOMMENDMONITORMODES_CONST pRecommendMonitorModes) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->RecommendMonitorModes(pRecommendMonitorModes);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERYVIDPNHWCAPABILITY)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    VfbQueryVidPnHWCapability(IN_CONST_HANDLE i_hAdapter, INOUT_PDXGKARG_QUERYVIDPNHWCAPABILITY io_pVidPnHWCaps) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(i_hAdapter);
    return device->QueryVidPnHWCapability(io_pVidPnHWCaps);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_PRESENTDISPLAYONLY)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    VfbPresentDisplayOnly(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_PRESENT_DISPLAYONLY pPresentDisplayOnly) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(hAdapter);
    return device->PresentDisplayOnly(pPresentDisplayOnly);
}

EXTERN_C DECLSPEC_PAGED_CODE _Check_return_
_Function_class_DXGK_(DXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbStopDeviceAndReleasePostDisplayOwnership(
        _In_ PVOID MiniportDeviceContext,
        _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
        _Out_ PDXGK_DISPLAY_INFORMATION DisplayInfo) {
    PAGED_CODE();

    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->StopDeviceAndReleasePostDisplayOwnership(TargetId, DisplayInfo);
}

EXTERN_C _Check_return_ _Function_class_DXGK_(DXGKDDI_SYSTEM_DISPLAY_ENABLE)
NTSTATUS VfbSystemDisplayEnable(
    _In_ PVOID MiniportDeviceContext,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _In_ PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS Flags,
    _Out_ UINT *Width,
    _Out_ UINT *Height,
    _Out_ D3DDDIFORMAT *ColorFormat) {
    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->SystemDisplayEnable(TargetId, Flags, Width, Height, ColorFormat);
}

EXTERN_C _Function_class_DXGK_(DXGKDDI_SYSTEM_DISPLAY_WRITE)
VOID VfbSystemDisplayWrite(
    _In_ PVOID MiniportDeviceContext,
    _In_reads_bytes_(SourceHeight *SourceStride) PVOID Source,
    _In_ UINT SourceWidth,
    _In_ UINT SourceHeight,
    _In_ UINT SourceStride,
    _In_ UINT PositionX,
    _In_ UINT PositionY) {
    auto device = static_cast<VfbDevice *>(MiniportDeviceContext);
    return device->SystemDisplayWrite(Source, SourceWidth, SourceHeight, SourceStride, PositionX, PositionY);
}

EXTERN_C _Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    PAGED_CODE();

    KMDDOD_INITIALIZATION_DATA InitialData{
        .Version = DXGKDDI_INTERFACE_VERSION,
        .DxgkDdiAddDevice = VfbAddDevice,
        .DxgkDdiStartDevice = VfbStartDevice,
        .DxgkDdiStopDevice = VfbStopDevice,
        .DxgkDdiRemoveDevice = VfbRemoveDevice,
        .DxgkDdiDispatchIoRequest = VfbDispatchIoRequest,
        .DxgkDdiInterruptRoutine = VfbInterruptRoutine,
        .DxgkDdiDpcRoutine = VfbDpcRoutine,
        .DxgkDdiQueryChildRelations = VfbQueryChildRelations,
        .DxgkDdiQueryChildStatus = VfbQueryChildStatus,
        .DxgkDdiQueryDeviceDescriptor = VfbQueryDeviceDescriptor,
        .DxgkDdiSetPowerState = VfbSetPowerState,
        // .DxgkDdiNotifyAcpiEvent
        .DxgkDdiResetDevice = VfbResetDevice,
        .DxgkDdiUnload = VfbUnload,
        // .DxgkDdiQueryInterface
        // .DxgkDdiControlEtwLogging
        .DxgkDdiQueryAdapterInfo = VfbQueryAdapterInfo,
        // .DxgkDdiSetPalette
        .DxgkDdiSetPointerPosition = VfbSetPointerPosition,
        .DxgkDdiSetPointerShape = VfbSetPointerShape,
        // .DxgkDdiEscape
        // .DxgkDdiCollectDbgInfo
        .DxgkDdiIsSupportedVidPn = VfbIsSupportedVidPn,
        .DxgkDdiRecommendFunctionalVidPn = VfbRecommendFunctionalVidPn,
        .DxgkDdiEnumVidPnCofuncModality = VfbEnumVidPnCofuncModality,
        .DxgkDdiSetVidPnSourceVisibility = VfbSetVidPnSourceVisibility,
        .DxgkDdiCommitVidPn = VfbCommitVidPn,
        .DxgkDdiUpdateActiveVidPnPresentPath = VfbUpdateActiveVidPnPresentPath,
        .DxgkDdiRecommendMonitorModes = VfbRecommendMonitorModes,
        // .DxgkDdiGetScanLine
        .DxgkDdiQueryVidPnHWCapability = VfbQueryVidPnHWCapability,
        .DxgkDdiPresentDisplayOnly = VfbPresentDisplayOnly,
        .DxgkDdiStopDeviceAndReleasePostDisplayOwnership = VfbStopDeviceAndReleasePostDisplayOwnership,
        .DxgkDdiSystemDisplayEnable = VfbSystemDisplayEnable,
        .DxgkDdiSystemDisplayWrite = VfbSystemDisplayWrite,
        // .DxgkDdiGetChildContainerId
        // .DxgkDdiControlInterrupt
        // .DxgkDdiSetPowerComponentFState
        // .DxgkDdiPowerRuntimeControlRequest
        // .DxgkDdiNotifySurpriseRemoval
    };
    return DxgkInitializeDisplayOnlyDriver(DriverObject, RegistryPath, &InitialData);
}
