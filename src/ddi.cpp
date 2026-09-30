extern "C" {
#include <ntddk.h>

#include <dispmprt.h>
}

#include "device.hpp"

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_ADD_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
    FbAddDevice(IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject, OUT_PPVOID MiniportDeviceContext) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    if (MiniportDeviceContext != nullptr) {
        *MiniportDeviceContext = nullptr;
    }
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_START_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS FbStartDevice(
        IN_CONST_PVOID MiniportDeviceContext,
        IN_PDXGK_START_INFO DxgkStartInfo,
        IN_PDXGKRNL_INTERFACE DxgkInterface,
        OUT_PULONG NumberOfVideoPresentSources,
        OUT_PULONG NumberOfChildren) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(DxgkStartInfo);
    UNREFERENCED_PARAMETER(DxgkInterface);
    if (NumberOfVideoPresentSources != nullptr) {
        *NumberOfVideoPresentSources = 0;
    }
    if (NumberOfChildren != nullptr) {
        *NumberOfChildren = 0;
    }
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_STOP_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS FbStopDevice(IN_CONST_PVOID MiniportDeviceContext) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_REMOVE_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS FbRemoveDevice(IN_CONST_PVOID MiniportDeviceContext) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_DISPATCH_IO_REQUEST)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS FbDispatchIoRequest(
        IN_CONST_PVOID MiniportDeviceContext,
        IN_ULONG VidPnSourceId,
        IN_PVIDEO_REQUEST_PACKET VideoRequestPacket) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(VidPnSourceId);
    UNREFERENCED_PARAMETER(VideoRequestPacket);
    return STATUS_NOT_IMPLEMENTED;
}

_Check_return_ _Function_class_DXGK_(DXGKDDI_INTERRUPT_ROUTINE)
    _IRQL_requires_DXGK_(HIGH_LEVEL) // HIGH_LEVEL is best approximation of DIRQL
    BOOLEAN FbInterruptRoutine(IN_CONST_PVOID MiniportDeviceContext, IN_ULONG MessageNumber) {
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(MessageNumber);
    return FALSE;
}

_Function_class_DXGK_(DXGKDDI_DPC_ROUTINE) _IRQL_requires_DXGK_(DISPATCH_LEVEL) VOID
    FbDpcRoutine(IN_CONST_PVOID MiniportDeviceContext) {
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERY_CHILD_RELATIONS)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS FbQueryChildRelations(
        IN_CONST_PVOID MiniportDeviceContext,
        _Inout_updates_bytes_(ChildRelationsSize) PDXGK_CHILD_DESCRIPTOR ChildRelations,
        _In_ ULONG ChildRelationsSize) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(ChildRelations);
    UNREFERENCED_PARAMETER(ChildRelationsSize);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERY_CHILD_STATUS)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS FbQueryChildStatus(
        IN_CONST_PVOID MiniportDeviceContext,
        INOUT_PDXGK_CHILD_STATUS ChildStatus,
        IN_BOOLEAN NonDestructiveOnly) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(ChildStatus);
    UNREFERENCED_PARAMETER(NonDestructiveOnly);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERY_DEVICE_DESCRIPTOR)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS FbQueryDeviceDescriptor(
        IN_CONST_PVOID MiniportDeviceContext,
        IN_ULONG ChildUid,
        INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(ChildUid);
    UNREFERENCED_PARAMETER(DeviceDescriptor);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_SET_POWER_STATE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS FbSetPowerState(
        IN_CONST_PVOID MiniportDeviceContext,
        IN_ULONG DeviceUid,
        IN_DEVICE_POWER_STATE DevicePowerState,
        IN_POWER_ACTION ActionType) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(DeviceUid);
    UNREFERENCED_PARAMETER(DevicePowerState);
    UNREFERENCED_PARAMETER(ActionType);
    return STATUS_NOT_IMPLEMENTED;
}

_Function_class_DXGK_(DXGKDDI_RESET_DEVICE) VOID FbResetDevice(IN_CONST_PVOID MiniportDeviceContext) {
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
}

DECLSPEC_PAGED_CODE _Function_class_DXGK_(DXGKDDI_UNLOAD) _IRQL_requires_DXGK_(PASSIVE_LEVEL) VOID FbUnload(VOID) {
    PAGED_CODE();
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERYADAPTERINFO)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    FbQueryAdapterInfo(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_QUERYADAPTERINFO pQueryAdapterInfo) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pQueryAdapterInfo);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_SETPOINTERPOSITION)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    FbSetPointerPosition(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_SETPOINTERPOSITION pSetPointerPosition) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pSetPointerPosition);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_SETPOINTERSHAPE)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    FbSetPointerShape(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_SETPOINTERSHAPE pSetPointerShape) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pSetPointerShape);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_ISSUPPORTEDVIDPN)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    FbIsSupportedVidPn(IN_CONST_HANDLE hAdapter, INOUT_PDXGKARG_ISSUPPORTEDVIDPN pIsSupportedVidPn) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pIsSupportedVidPn);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_RECOMMENDFUNCTIONALVIDPN)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY FbRecommendFunctionalVidPn(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_RECOMMENDFUNCTIONALVIDPN_CONST pRecommendFunctionalVidPn) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pRecommendFunctionalVidPn);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_ENUMVIDPNCOFUNCMODALITY)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY FbEnumVidPnCofuncModality(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pEnumCofuncModality);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_SETVIDPNSOURCEVISIBILITY)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY FbSetVidPnSourceVisibility(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_SETVIDPNSOURCEVISIBILITY pSetVidPnSourceVisibility) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pSetVidPnSourceVisibility);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_COMMITVIDPN)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    FbCommitVidPn(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_COMMITVIDPN_CONST pCommitVidPn) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pCommitVidPn);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY FbUpdateActiveVidPnPresentPath(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_UPDATEACTIVEVIDPNPRESENTPATH_CONST pUpdateActiveVidPnPresentPath) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pUpdateActiveVidPnPresentPath);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_RECOMMENDMONITORMODES)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY FbRecommendMonitorModes(
        IN_CONST_HANDLE hAdapter,
        IN_CONST_PDXGKARG_RECOMMENDMONITORMODES_CONST pRecommendMonitorModes) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pRecommendMonitorModes);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_QUERYVIDPNHWCAPABILITY)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    FbQueryVidPnHWCapability(IN_CONST_HANDLE i_hAdapter, INOUT_PDXGKARG_QUERYVIDPNHWCAPABILITY io_pVidPnHWCaps) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(i_hAdapter);
    UNREFERENCED_PARAMETER(io_pVidPnHWCaps);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_PRESENTDISPLAYONLY)
    _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS APIENTRY
    FbPresentDisplayOnly(IN_CONST_HANDLE hAdapter, IN_CONST_PDXGKARG_PRESENT_DISPLAYONLY pPresentDisplayOnly) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pPresentDisplayOnly);
    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS FbStopDeviceAndReleasePostDisplayOwnership(
        _In_ PVOID MiniportDeviceContext,
        _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
        _Out_ PDXGK_DISPLAY_INFORMATION DisplayInfo) {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(TargetId);
    UNREFERENCED_PARAMETER(DisplayInfo);
    return STATUS_NOT_IMPLEMENTED;
}

_Check_return_ _Function_class_DXGK_(DXGKDDI_SYSTEM_DISPLAY_ENABLE)
NTSTATUS FbSystemDisplayEnable(
    _In_ PVOID MiniportDeviceContext,
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _In_ PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS Flags,
    _Out_ UINT *Width,
    _Out_ UINT *Height,
    _Out_ D3DDDIFORMAT *ColorFormat) {
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(TargetId);
    UNREFERENCED_PARAMETER(Flags);
    if (Width != nullptr) {
        *Width = 0;
    }
    if (Height != nullptr) {
        *Height = 0;
    }
    if (ColorFormat != nullptr) {
        *ColorFormat = D3DDDIFMT_UNKNOWN;
    }
    return STATUS_NOT_IMPLEMENTED;
}

_Function_class_DXGK_(DXGKDDI_SYSTEM_DISPLAY_WRITE) VOID FbSystemDisplayWrite(
    _In_ PVOID MiniportDeviceContext,
    _In_reads_bytes_(SourceHeight *SourceStride) PVOID Source,
    _In_ UINT SourceWidth,
    _In_ UINT SourceHeight,
    _In_ UINT SourceStride,
    _In_ UINT PositionX,
    _In_ UINT PositionY) {
    UNREFERENCED_PARAMETER(MiniportDeviceContext);
    UNREFERENCED_PARAMETER(Source);
    UNREFERENCED_PARAMETER(SourceWidth);
    UNREFERENCED_PARAMETER(SourceHeight);
    UNREFERENCED_PARAMETER(SourceStride);
    UNREFERENCED_PARAMETER(PositionX);
    UNREFERENCED_PARAMETER(PositionY);
}

DECLSPEC_PAGED_CODE EXTERN_C NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    PAGED_CODE();

    KMDDOD_INITIALIZATION_DATA InitialData{
        .Version = DXGKDDI_INTERFACE_VERSION_WIN8,
        .DxgkDdiAddDevice = FbAddDevice,
        .DxgkDdiStartDevice = FbStartDevice,
        .DxgkDdiStopDevice = FbStopDevice,
        .DxgkDdiRemoveDevice = FbRemoveDevice,
        .DxgkDdiDispatchIoRequest = FbDispatchIoRequest,
        .DxgkDdiInterruptRoutine = FbInterruptRoutine,
        .DxgkDdiDpcRoutine = FbDpcRoutine,
        .DxgkDdiQueryChildRelations = FbQueryChildRelations,
        .DxgkDdiQueryChildStatus = FbQueryChildStatus,
        .DxgkDdiQueryDeviceDescriptor = FbQueryDeviceDescriptor,
        .DxgkDdiSetPowerState = FbSetPowerState,
        // .DxgkDdiNotifyAcpiEvent = FbNotifyAcpiEvent,
        .DxgkDdiResetDevice = FbResetDevice,
        .DxgkDdiUnload = FbUnload,
        // .DxgkDdiQueryInterface = FbQueryInterface,
        // .DxgkDdiControlEtwLogging = FbControlEtwLogging,
        .DxgkDdiQueryAdapterInfo = FbQueryAdapterInfo,
        // .DxgkDdiSetPalette = FbSetPalette,
        .DxgkDdiSetPointerPosition = FbSetPointerPosition,
        .DxgkDdiSetPointerShape = FbSetPointerShape,
        // .DxgkDdiEscape = FbEscape,
        // .DxgkDdiCollectDbgInfo = FbCollectDbgInfo,
        .DxgkDdiIsSupportedVidPn = FbIsSupportedVidPn,
        .DxgkDdiRecommendFunctionalVidPn = FbRecommendFunctionalVidPn,
        .DxgkDdiEnumVidPnCofuncModality = FbEnumVidPnCofuncModality,
        .DxgkDdiSetVidPnSourceVisibility = FbSetVidPnSourceVisibility,
        .DxgkDdiCommitVidPn = FbCommitVidPn,
        .DxgkDdiUpdateActiveVidPnPresentPath = FbUpdateActiveVidPnPresentPath,
        .DxgkDdiRecommendMonitorModes = FbRecommendMonitorModes,
        // .DxgkDdiGetScanLine = FbGetScanLine,
        .DxgkDdiQueryVidPnHWCapability = FbQueryVidPnHWCapability,
        .DxgkDdiPresentDisplayOnly = FbPresentDisplayOnly,
        .DxgkDdiStopDeviceAndReleasePostDisplayOwnership = FbStopDeviceAndReleasePostDisplayOwnership,
        .DxgkDdiSystemDisplayEnable = FbSystemDisplayEnable,
        .DxgkDdiSystemDisplayWrite = FbSystemDisplayWrite,
        // .DxgkDdiGetChildContainerId
        // .DxgkDdiControlInterrupt
        // .DxgkDdiSetPowerComponentFState
        // .DxgkDdiPowerRuntimeControlRequest
        // .DxgkDdiNotifySurpriseRemoval = FbNotifySurpriseRemoval,
    };
    return DxgkInitializeDisplayOnlyDriver(DriverObject, RegistryPath, &InitialData);
}
