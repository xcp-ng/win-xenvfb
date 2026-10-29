#pragma once

#include "xenvfb.hpp"
#include <xenklib/xenbus.hpp>
#include <xenklib/allocator.hpp>
#include "frontends.hpp"

class VfbDevice {
public:
    explicit VfbDevice(IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject) noexcept : _pdo(PhysicalDeviceObject) {}
    VfbDevice(const VfbDevice &) = delete;
    VfbDevice &operator=(const VfbDevice &) = delete;
    VfbDevice(VfbDevice &&) = delete;
    VfbDevice &operator=(VfbDevice &&) = delete;
    ~VfbDevice() {}

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS StartDevice(
        IN_PDXGK_START_INFO DxgkStartInfo,
        IN_PDXGKRNL_INTERFACE DxgkInterface,
        OUT_PULONG NumberOfVideoPresentSources,
        OUT_PULONG NumberOfChildren);

    // startup sequence
    void StartDxgk(IN_PDXGKRNL_INTERFACE DxgkInterface) {
        _dxgk = *DxgkInterface;
    }
    void StopDxgk() {
        _dxgk = {};
    }
    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS StartXen();
    DECLSPEC_PAGED_CODE _IRQL_requires_DXGK_(PASSIVE_LEVEL) void StopXen();

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS StopDevice();

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        DispatchIoRequest(IN_ULONG VidPnSourceId, IN_PVIDEO_REQUEST_PACKET VideoRequestPacket);

    _Check_return_ _IRQL_requires_DXGK_(HIGH_LEVEL) BOOLEAN InterruptRoutine(IN_ULONG MessageNumber);

    _IRQL_requires_DXGK_(DISPATCH_LEVEL) VOID DpcRoutine();

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS QueryChildRelations(
        _Inout_updates_bytes_(ChildRelationsSize) PDXGK_CHILD_DESCRIPTOR ChildRelations,
        _In_ ULONG ChildRelationsSize);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        QueryChildStatus(INOUT_PDXGK_CHILD_STATUS ChildStatus, IN_BOOLEAN NonDestructiveOnly);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        QueryDeviceDescriptor(IN_ULONG ChildUid, INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        SetPowerState(IN_ULONG DeviceUid, IN_DEVICE_POWER_STATE DevicePowerState, IN_POWER_ACTION ActionType);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        QueryAdapterInfo(IN_CONST_PDXGKARG_QUERYADAPTERINFO pQueryAdapterInfo);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        SetPointerPosition(IN_CONST_PDXGKARG_SETPOINTERPOSITION pSetPointerPosition);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        SetPointerShape(IN_CONST_PDXGKARG_SETPOINTERSHAPE pSetPointerShape);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        IsSupportedVidPn(INOUT_PDXGKARG_ISSUPPORTEDVIDPN pIsSupportedVidPn);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        RecommendFunctionalVidPn(IN_CONST_PDXGKARG_RECOMMENDFUNCTIONALVIDPN_CONST pRecommendFunctionalVidPn);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        EnumVidPnCofuncModality(IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        SetVidPnSourceVisibility(IN_CONST_PDXGKARG_SETVIDPNSOURCEVISIBILITY pSetVidPnSourceVisibility);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        CommitVidPn(IN_CONST_PDXGKARG_COMMITVIDPN_CONST pCommitVidPn);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS UpdateActiveVidPnPresentPath(
        IN_CONST_PDXGKARG_UPDATEACTIVEVIDPNPRESENTPATH_CONST pUpdateActiveVidPnPresentPath);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        RecommendMonitorModes(IN_CONST_PDXGKARG_RECOMMENDMONITORMODES_CONST pRecommendMonitorModes);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        QueryVidPnHWCapability(INOUT_PDXGKARG_QUERYVIDPNHWCAPABILITY io_pVidPnHWCaps);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        PresentDisplayOnly(IN_CONST_PDXGKARG_PRESENT_DISPLAYONLY pPresentDisplayOnly);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        StopDeviceAndReleasePostDisplayOwnership(
            _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
            _Out_ PDXGK_DISPLAY_INFORMATION DisplayInfo);

    _Check_return_ NTSTATUS SystemDisplayEnable(
        _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
        _In_ PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS Flags,
        _Out_ UINT *Width,
        _Out_ UINT *Height,
        _Out_ D3DDDIFORMAT *ColorFormat);

    VOID SystemDisplayWrite(
        _In_reads_bytes_(SourceHeight *SourceStride) PVOID Source,
        _In_ UINT SourceWidth,
        _In_ UINT SourceHeight,
        _In_ UINT SourceStride,
        _In_ UINT PositionX,
        _In_ UINT PositionY);

    VOID ResetDevice();

private:
    const PDEVICE_OBJECT _pdo = nullptr;

    DXGKRNL_INTERFACE _dxgk{};

    DebugInterface _debug{};
    EvtchnInterface _evtchn{};
    StoreInterface _store{};
    SuspendInterface _suspend{};

    UniqueVfb<VfbFrontendManager> _fm;
    DEVICE_POWER_STATE _powerState = PowerDeviceD0;

    bool _started = false;
};
