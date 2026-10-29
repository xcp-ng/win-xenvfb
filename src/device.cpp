#include "xenvfb.hpp"
#include <xenklib/xenbus.hpp>
#include "device.hpp"
#include "frontends.hpp"

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::StartDevice(
    IN_PDXGK_START_INFO DxgkStartInfo,
    IN_PDXGKRNL_INTERFACE DxgkInterface,
    OUT_PULONG NumberOfVideoPresentSources,
    OUT_PULONG NumberOfChildren) {
    PAGED_CODE();

    NTSTATUS status;

    UNREFERENCED_PARAMETER(DxgkStartInfo);
    UNREFERENCED_PARAMETER(NumberOfVideoPresentSources);
    UNREFERENCED_PARAMETER(NumberOfChildren);

    if (_started) {
        return STATUS_UNSUCCESSFUL;
    }

    if (DxgkInterface->Version < DXGKDDI_INTERFACE_VERSION || DxgkInterface->Size < sizeof(*DxgkInterface)) {
        return STATUS_NOT_SUPPORTED;
    }

    StartDxgk(DxgkInterface);
    auto cleanupDxgk = wil::scope_exit([&]() { StopDxgk(); });

    status = StartXen();
    auto cleanupXen = wil::scope_exit([&]() { StopXen(); });
    if (!NT_SUCCESS(status)) {
        return status;
    }

    {
        xenklib::StoreHandle storeHandle;
        status = _store.Acquire(storeHandle);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        _fm = MakeUniqueVfb<VfbFrontendManager>(wistd::move(storeHandle));
    }
    auto cleanupFm = wil::scope_exit([&]() { _fm.release(); });

    status = _fm->Initialize();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    *NumberOfVideoPresentSources = VfbFrontendManager::MaxViews;
    *NumberOfChildren = VfbFrontendManager::MaxChildren;

    cleanupFm.release();
    cleanupXen.release();
    cleanupDxgk.release();
    _started = true;
    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::StartXen() {
    PAGED_CODE();

    NTSTATUS status;

    xenklib::DebugInterface debug;
    status = xenklib::DebugInterface::Query(debug, _pdo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    xenklib::EvtchnInterface evtchn;
    status = xenklib::EvtchnInterface::Query(evtchn, _pdo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    xenklib::StoreInterface store;
    status = xenklib::StoreInterface::Query(store, _pdo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    xenklib::SuspendInterface suspend;
    status = xenklib::SuspendInterface::Query(suspend, _pdo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    _debug = wistd::move(debug);
    _evtchn = wistd::move(evtchn);
    _store = wistd::move(store);
    _suspend = wistd::move(suspend);
    return status;
}

DECLSPEC_PAGED_CODE _IRQL_requires_DXGK_(PASSIVE_LEVEL) void VfbDevice::StopXen() {
    PAGED_CODE();

    _suspend.Reset();
    _store.Reset();
    _evtchn.Reset();
    _debug.Reset();
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::StopDevice() {
    PAGED_CODE();

    _started = false;
    _fm.release();
    StopXen();
    StopDxgk();

    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::DispatchIoRequest(IN_ULONG VidPnSourceId, IN_PVIDEO_REQUEST_PACKET VideoRequestPacket) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(VidPnSourceId);
    UNREFERENCED_PARAMETER(VideoRequestPacket);

    return STATUS_NOT_IMPLEMENTED;
}

_Check_return_ _IRQL_requires_DXGK_(HIGH_LEVEL)
BOOLEAN VfbDevice::InterruptRoutine(IN_ULONG MessageNumber) {
    UNREFERENCED_PARAMETER(MessageNumber);

    return FALSE;
}

_IRQL_requires_DXGK_(DISPATCH_LEVEL) VOID VfbDevice::DpcRoutine() {
    _dxgk.DxgkCbNotifyDpc(_dxgk.DeviceHandle);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::QueryChildRelations(
    _Inout_updates_bytes_(ChildRelationsSize) PDXGK_CHILD_DESCRIPTOR ChildRelations,
    _In_ ULONG ChildRelationsSize) {
    PAGED_CODE();

    if (!ChildRelations) {
        return STATUS_INVALID_PARAMETER;
    }
    if (ChildRelationsSize / sizeof(*ChildRelations) != VfbFrontendManager::MaxChildren + 1) {
        return STATUS_INVALID_PARAMETER;
    }

    for (ULONG i = 0; i < VfbFrontendManager::MaxChildren; i++) {
        ChildRelations[i] = DXGK_CHILD_DESCRIPTOR{
            .ChildDeviceType = TypeVideoOutput,
            .ChildCapabilities =
                {
                    .Type =
                        {
                            .VideoOutput =
                                {
                                    .InterfaceTechnology = D3DKMDT_VOT_INTERNAL,
                                    .MonitorOrientationAwareness = D3DKMDT_MOA_NONE,
                                    .SupportsSdtvModes = FALSE,
                                },
                        },
                    .HpdAwareness = HpdAwarenessInterruptible,
                },
            .AcpiUid = 0,
            .ChildUid = i,
        };
    }

    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::QueryChildStatus(INOUT_PDXGK_CHILD_STATUS ChildStatus, IN_BOOLEAN NonDestructiveOnly) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(NonDestructiveOnly);

    if (!ChildStatus) {
        return STATUS_INVALID_PARAMETER;
    }
    if (ChildStatus->Type != StatusConnection) {
        return STATUS_NOT_SUPPORTED;
    }

    ChildStatus->HotPlug.Connected = _fm->HasChild(ChildStatus->ChildUid);
    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::QueryDeviceDescriptor(IN_ULONG ChildUid, INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) {
    PAGED_CODE();

    return _fm->QueryDeviceDescriptor(ChildUid, DeviceDescriptor);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::SetPowerState(IN_ULONG DeviceUid, IN_DEVICE_POWER_STATE DevicePowerState, IN_POWER_ACTION ActionType) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DeviceUid);
    UNREFERENCED_PARAMETER(DevicePowerState);
    UNREFERENCED_PARAMETER(ActionType);

    if (DeviceUid != DISPLAY_ADAPTER_HW_ID) {
        return _fm->SetPowerState(DeviceUid, DevicePowerState, ActionType);
    }

    // for us
    if (DevicePowerState == PowerDeviceD0) {
        // TODO

        if (_powerState == PowerDeviceD3) {
            DXGKARG_SETVIDPNSOURCEVISIBILITY Visibility;
            Visibility.VidPnSourceId = D3DDDI_ID_ALL;
            Visibility.Visible = FALSE;
            SetVidPnSourceVisibility(&Visibility);
        }
    }

    _powerState = DevicePowerState;
    return STATUS_SUCCESS;
}

VOID VfbDevice::ResetDevice() {}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::QueryAdapterInfo(IN_CONST_PDXGKARG_QUERYADAPTERINFO pQueryAdapterInfo) {
    PAGED_CODE();

    if (pQueryAdapterInfo->Type != DXGKQAITYPE_DRIVERCAPS) {
        return STATUS_NOT_SUPPORTED;
    }
    if (pQueryAdapterInfo->OutputDataSize < sizeof(DXGK_DRIVERCAPS)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    auto driverCaps = static_cast<DXGK_DRIVERCAPS *>(pQueryAdapterInfo->pOutputData);
    *driverCaps = {
        .HighestAcceptableAddress =
            {
                .QuadPart = -1,
            },
        .WDDMVersion = DXGKDDI_WDDMv1_2,
        .SupportNonVGA = TRUE,
        .SupportSmoothRotation = TRUE,
    };
    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::SetPointerPosition(IN_CONST_PDXGKARG_SETPOINTERPOSITION pSetPointerPosition) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pSetPointerPosition);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::SetPointerShape(IN_CONST_PDXGKARG_SETPOINTERSHAPE pSetPointerShape) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pSetPointerShape);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::IsSupportedVidPn(INOUT_PDXGKARG_ISSUPPORTEDVIDPN pIsSupportedVidPn) {
    PAGED_CODE();

    NTSTATUS status;

    if (!pIsSupportedVidPn) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!pIsSupportedVidPn->hDesiredVidPn) {
        return STATUS_SUCCESS;
    }

    const DXGK_VIDPN_INTERFACE *vidPnInterface;
    status = _dxgk.DxgkCbQueryVidPnInterface(
        pIsSupportedVidPn->hDesiredVidPn,
        DXGK_VIDPN_INTERFACE_VERSION_V1,
        &vidPnInterface);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    D3DKMDT_HVIDPNTOPOLOGY vidPnTopology;
    const DXGK_VIDPNTOPOLOGY_INTERFACE *vidPnTopologyInterface;
    status = vidPnInterface->pfnGetTopology(pIsSupportedVidPn->hDesiredVidPn, &vidPnTopology, &vidPnTopologyInterface);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return _fm->IsSupportedVidPn(pIsSupportedVidPn, vidPnInterface, vidPnTopology, vidPnTopologyInterface);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::RecommendFunctionalVidPn(IN_CONST_PDXGKARG_RECOMMENDFUNCTIONALVIDPN_CONST pRecommendFunctionalVidPn) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pRecommendFunctionalVidPn);

    return STATUS_GRAPHICS_NO_RECOMMENDED_FUNCTIONAL_VIDPN;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::EnumVidPnCofuncModality(IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality) {
    PAGED_CODE();

    NTSTATUS status;

    if (!pEnumCofuncModality || !pEnumCofuncModality->hConstrainingVidPn) {
        return STATUS_INVALID_PARAMETER;
    }

    const DXGK_VIDPN_INTERFACE *vidPnInterface;
    status = _dxgk.DxgkCbQueryVidPnInterface(
        pEnumCofuncModality->hConstrainingVidPn,
        DXGK_VIDPN_INTERFACE_VERSION_V1,
        &vidPnInterface);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    D3DKMDT_HVIDPNTOPOLOGY vidPnTopology;
    const DXGK_VIDPNTOPOLOGY_INTERFACE *vidPnTopologyInterface;
    status = vidPnInterface->pfnGetTopology(
        pEnumCofuncModality->hConstrainingVidPn,
        &vidPnTopology,
        &vidPnTopologyInterface);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return _fm->EnumVidPnCofuncModality(pEnumCofuncModality, vidPnInterface, vidPnTopology, vidPnTopologyInterface);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::SetVidPnSourceVisibility(IN_CONST_PDXGKARG_SETVIDPNSOURCEVISIBILITY pSetVidPnSourceVisibility) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pSetVidPnSourceVisibility);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::CommitVidPn(IN_CONST_PDXGKARG_COMMITVIDPN_CONST pCommitVidPn) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pCommitVidPn);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::UpdateActiveVidPnPresentPath(
    IN_CONST_PDXGKARG_UPDATEACTIVEVIDPNPRESENTPATH_CONST pUpdateActiveVidPnPresentPath) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pUpdateActiveVidPnPresentPath);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::RecommendMonitorModes(IN_CONST_PDXGKARG_RECOMMENDMONITORMODES_CONST pRecommendMonitorModes) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pRecommendMonitorModes);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::QueryVidPnHWCapability(INOUT_PDXGKARG_QUERYVIDPNHWCAPABILITY io_pVidPnHWCaps) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(io_pVidPnHWCaps);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::PresentDisplayOnly(IN_CONST_PDXGKARG_PRESENT_DISPLAYONLY pPresentDisplayOnly) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pPresentDisplayOnly);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::StopDeviceAndReleasePostDisplayOwnership(
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _Out_ PDXGK_DISPLAY_INFORMATION DisplayInfo) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(TargetId);
    UNREFERENCED_PARAMETER(DisplayInfo);

    return STATUS_NOT_IMPLEMENTED;
}

_Check_return_ NTSTATUS VfbDevice::SystemDisplayEnable(
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _In_ PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS Flags,
    _Out_ UINT *Width,
    _Out_ UINT *Height,
    _Out_ D3DDDIFORMAT *ColorFormat) {
    UNREFERENCED_PARAMETER(TargetId);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(Width);
    UNREFERENCED_PARAMETER(Height);
    UNREFERENCED_PARAMETER(ColorFormat);

    return STATUS_NOT_IMPLEMENTED;
}

VOID VfbDevice::SystemDisplayWrite(
    _In_reads_bytes_(SourceHeight *SourceStride) PVOID Source,
    _In_ UINT SourceWidth,
    _In_ UINT SourceHeight,
    _In_ UINT SourceStride,
    _In_ UINT PositionX,
    _In_ UINT PositionY) {
    UNREFERENCED_PARAMETER(Source);
    UNREFERENCED_PARAMETER(SourceWidth);
    UNREFERENCED_PARAMETER(SourceHeight);
    UNREFERENCED_PARAMETER(SourceStride);
    UNREFERENCED_PARAMETER(PositionX);
    UNREFERENCED_PARAMETER(PositionY);
    // TODO
}
