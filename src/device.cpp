#include "device.hpp"
#include "xenbus.hpp"
#include "vfb.hpp"
#include "frontends.hpp"

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::StartDevice(
    IN_PDXGK_START_INFO DxgkStartInfo,
    IN_PDXGKRNL_INTERFACE DxgkInterface,
    OUT_PULONG NumberOfVideoPresentSources,
    OUT_PULONG NumberOfChildren) noexcept {
    NTSTATUS Status;

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

    Status = StartXen();
    auto cleanupXen = wil::scope_exit([&]() { StopXen(); });
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    {
        StoreHandle storeHandle;
        Status = _store.Acquire(storeHandle);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        _fm = MakeUniqueVfb<VfbFrontendManager>(wistd::move(storeHandle));
    }
    auto cleanupFm = wil::scope_exit([&]() { _fm.release(); });

    Status = _fm->Initialize();
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    *NumberOfVideoPresentSources = VfbMaxViews;
    *NumberOfChildren = VfbMaxChildren;

    cleanupFm.release();
    cleanupXen.release();
    cleanupDxgk.release();
    _started = true;
    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::StartXen() {
    NTSTATUS Status;

    DebugInterface debug;
    Status = DebugInterface::Query(debug, _pdo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    EvtchnInterface evtchn;
    Status = EvtchnInterface::Query(evtchn, _pdo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    StoreInterface store;
    Status = StoreInterface::Query(store, _pdo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    SuspendInterface suspend;
    Status = SuspendInterface::Query(suspend, _pdo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    _debug = wistd::move(debug);
    _evtchn = wistd::move(evtchn);
    _store = wistd::move(store);
    _suspend = wistd::move(suspend);
    return Status;
}

DECLSPEC_PAGED_CODE _IRQL_requires_DXGK_(PASSIVE_LEVEL) void VfbDevice::StopXen() {
    _suspend.Reset();
    _store.Reset();
    _evtchn.Reset();
    _debug.Reset();
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::StopDevice() noexcept {
    _started = false;
    _fm.release();
    StopXen();
    StopDxgk();

    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::DispatchIoRequest(IN_ULONG VidPnSourceId, IN_PVIDEO_REQUEST_PACKET VideoRequestPacket) noexcept {
    UNREFERENCED_PARAMETER(VidPnSourceId);
    UNREFERENCED_PARAMETER(VideoRequestPacket);

    return STATUS_NOT_IMPLEMENTED;
}

_Check_return_ _IRQL_requires_DXGK_(HIGH_LEVEL)
BOOLEAN VfbDevice::InterruptRoutine(IN_ULONG MessageNumber) noexcept {
    UNREFERENCED_PARAMETER(MessageNumber);

    return FALSE;
}

_IRQL_requires_DXGK_(DISPATCH_LEVEL) VOID VfbDevice::DpcRoutine() noexcept {
    _dxgk.DxgkCbNotifyDpc(_dxgk.DeviceHandle);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::QueryChildRelations(
    _Inout_updates_bytes_(ChildRelationsSize) PDXGK_CHILD_DESCRIPTOR ChildRelations,
    _In_ ULONG ChildRelationsSize) noexcept {
    if (!ChildRelations) {
        return STATUS_INVALID_PARAMETER;
    }
    if (ChildRelationsSize / sizeof(*ChildRelations) != VfbMaxChildren + 1) {
        return STATUS_INVALID_PARAMETER;
    }

    for (ULONG i = 0; i < VfbMaxChildren; i++) {
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
VfbDevice::QueryChildStatus(INOUT_PDXGK_CHILD_STATUS ChildStatus, IN_BOOLEAN NonDestructiveOnly) noexcept {
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
VfbDevice::QueryDeviceDescriptor(IN_ULONG ChildUid, INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) noexcept {
    return _fm->QueryDeviceDescriptor(ChildUid, DeviceDescriptor);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::SetPowerState(
    IN_ULONG DeviceUid,
    IN_DEVICE_POWER_STATE DevicePowerState,
    IN_POWER_ACTION ActionType) noexcept {
    UNREFERENCED_PARAMETER(DeviceUid);
    UNREFERENCED_PARAMETER(DevicePowerState);
    UNREFERENCED_PARAMETER(ActionType);

    if (DeviceUid != DISPLAY_ADAPTER_HW_ID) {
        return _fm->SetPowerState(DeviceUid, DevicePowerState, ActionType);
    }

    // for us
    if (DevicePowerState == PowerDeviceD0) {
        if (_powerState == PowerDeviceD3) {
            DXGKARG_SETVIDPNSOURCEVISIBILITY Visibility;
            Visibility.VidPnSourceId = D3DDDI_ID_ALL;
            Visibility.Visible = FALSE;
            SetVidPnSourceVisibility(&Visibility);
        }
        // TODO
        return STATUS_NOT_IMPLEMENTED;
    }

    _powerState = DevicePowerState;
    return STATUS_SUCCESS;
}

VOID VfbDevice::ResetDevice() noexcept {}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::QueryAdapterInfo(IN_CONST_PDXGKARG_QUERYADAPTERINFO pQueryAdapterInfo) noexcept {
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
VfbDevice::SetPointerPosition(IN_CONST_PDXGKARG_SETPOINTERPOSITION pSetPointerPosition) noexcept {
    UNREFERENCED_PARAMETER(pSetPointerPosition);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::SetPointerShape(IN_CONST_PDXGKARG_SETPOINTERSHAPE pSetPointerShape) noexcept {
    UNREFERENCED_PARAMETER(pSetPointerShape);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::IsSupportedVidPn(INOUT_PDXGKARG_ISSUPPORTEDVIDPN pIsSupportedVidPn) noexcept {
    UNREFERENCED_PARAMETER(pIsSupportedVidPn);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::RecommendFunctionalVidPn(
    IN_CONST_PDXGKARG_RECOMMENDFUNCTIONALVIDPN_CONST pRecommendFunctionalVidPn) noexcept {
    UNREFERENCED_PARAMETER(pRecommendFunctionalVidPn);

    return STATUS_GRAPHICS_NO_RECOMMENDED_FUNCTIONAL_VIDPN;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::EnumVidPnCofuncModality(IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality) noexcept {
    UNREFERENCED_PARAMETER(pEnumCofuncModality);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::SetVidPnSourceVisibility(IN_CONST_PDXGKARG_SETVIDPNSOURCEVISIBILITY pSetVidPnSourceVisibility) noexcept {
    UNREFERENCED_PARAMETER(pSetVidPnSourceVisibility);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::CommitVidPn(IN_CONST_PDXGKARG_COMMITVIDPN_CONST pCommitVidPn) noexcept {
    UNREFERENCED_PARAMETER(pCommitVidPn);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS VfbDevice::UpdateActiveVidPnPresentPath(
    IN_CONST_PDXGKARG_UPDATEACTIVEVIDPNPRESENTPATH_CONST pUpdateActiveVidPnPresentPath) noexcept {
    UNREFERENCED_PARAMETER(pUpdateActiveVidPnPresentPath);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::RecommendMonitorModes(IN_CONST_PDXGKARG_RECOMMENDMONITORMODES_CONST pRecommendMonitorModes) noexcept {
    UNREFERENCED_PARAMETER(pRecommendMonitorModes);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::QueryVidPnHWCapability(INOUT_PDXGKARG_QUERYVIDPNHWCAPABILITY io_pVidPnHWCaps) noexcept {
    UNREFERENCED_PARAMETER(io_pVidPnHWCaps);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::PresentDisplayOnly(IN_CONST_PDXGKARG_PRESENT_DISPLAYONLY pPresentDisplayOnly) noexcept {
    UNREFERENCED_PARAMETER(pPresentDisplayOnly);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbDevice::StopDeviceAndReleasePostDisplayOwnership(
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _Out_ PDXGK_DISPLAY_INFORMATION DisplayInfo) noexcept {
    UNREFERENCED_PARAMETER(TargetId);
    UNREFERENCED_PARAMETER(DisplayInfo);

    return STATUS_NOT_IMPLEMENTED;
}

_Check_return_ NTSTATUS VfbDevice::SystemDisplayEnable(
    _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _In_ PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS Flags,
    _Out_ UINT *Width,
    _Out_ UINT *Height,
    _Out_ D3DDDIFORMAT *ColorFormat) noexcept {
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
    _In_ UINT PositionY) noexcept {
    UNREFERENCED_PARAMETER(Source);
    UNREFERENCED_PARAMETER(SourceWidth);
    UNREFERENCED_PARAMETER(SourceHeight);
    UNREFERENCED_PARAMETER(SourceStride);
    UNREFERENCED_PARAMETER(PositionX);
    UNREFERENCED_PARAMETER(PositionY);
    // TODO
}
