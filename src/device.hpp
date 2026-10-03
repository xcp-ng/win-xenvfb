#pragma once

#include "common.hpp"
#include "xenbus.hpp"
#include "allocator.hpp"
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
        OUT_PULONG NumberOfChildren) noexcept;

    // startup sequence
    void StartDxgk(IN_PDXGKRNL_INTERFACE DxgkInterface) {
        _dxgk = *DxgkInterface;
    }
    void StopDxgk() {
        _dxgk = {};
    }
    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS StartXen();
    DECLSPEC_PAGED_CODE _IRQL_requires_DXGK_(PASSIVE_LEVEL) void StopXen();

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS StopDevice() noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        DispatchIoRequest(IN_ULONG VidPnSourceId, IN_PVIDEO_REQUEST_PACKET VideoRequestPacket) noexcept;

    _Check_return_ _IRQL_requires_DXGK_(HIGH_LEVEL) BOOLEAN InterruptRoutine(IN_ULONG MessageNumber) noexcept;

    _IRQL_requires_DXGK_(DISPATCH_LEVEL) VOID DpcRoutine() noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS QueryChildRelations(
        _Inout_updates_bytes_(ChildRelationsSize) PDXGK_CHILD_DESCRIPTOR ChildRelations,
        _In_ ULONG ChildRelationsSize) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        QueryChildStatus(INOUT_PDXGK_CHILD_STATUS ChildStatus, IN_BOOLEAN NonDestructiveOnly) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        QueryDeviceDescriptor(IN_ULONG ChildUid, INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        SetPowerState(IN_ULONG DeviceUid, IN_DEVICE_POWER_STATE DevicePowerState, IN_POWER_ACTION ActionType) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        QueryAdapterInfo(IN_CONST_PDXGKARG_QUERYADAPTERINFO pQueryAdapterInfo) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        SetPointerPosition(IN_CONST_PDXGKARG_SETPOINTERPOSITION pSetPointerPosition) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        SetPointerShape(IN_CONST_PDXGKARG_SETPOINTERSHAPE pSetPointerShape) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        IsSupportedVidPn(INOUT_PDXGKARG_ISSUPPORTEDVIDPN pIsSupportedVidPn) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        RecommendFunctionalVidPn(IN_CONST_PDXGKARG_RECOMMENDFUNCTIONALVIDPN_CONST pRecommendFunctionalVidPn) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        EnumVidPnCofuncModality(IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        SetVidPnSourceVisibility(IN_CONST_PDXGKARG_SETVIDPNSOURCEVISIBILITY pSetVidPnSourceVisibility) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        CommitVidPn(IN_CONST_PDXGKARG_COMMITVIDPN_CONST pCommitVidPn) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS UpdateActiveVidPnPresentPath(
        IN_CONST_PDXGKARG_UPDATEACTIVEVIDPNPRESENTPATH_CONST pUpdateActiveVidPnPresentPath) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        RecommendMonitorModes(IN_CONST_PDXGKARG_RECOMMENDMONITORMODES_CONST pRecommendMonitorModes) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        QueryVidPnHWCapability(INOUT_PDXGKARG_QUERYVIDPNHWCAPABILITY io_pVidPnHWCaps) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        PresentDisplayOnly(IN_CONST_PDXGKARG_PRESENT_DISPLAYONLY pPresentDisplayOnly) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        StopDeviceAndReleasePostDisplayOwnership(
            _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
            _Out_ PDXGK_DISPLAY_INFORMATION DisplayInfo) noexcept;

    _Check_return_ NTSTATUS SystemDisplayEnable(
        _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
        _In_ PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS Flags,
        _Out_ UINT *Width,
        _Out_ UINT *Height,
        _Out_ D3DDDIFORMAT *ColorFormat) noexcept;

    VOID SystemDisplayWrite(
        _In_reads_bytes_(SourceHeight *SourceStride) PVOID Source,
        _In_ UINT SourceWidth,
        _In_ UINT SourceHeight,
        _In_ UINT SourceStride,
        _In_ UINT PositionX,
        _In_ UINT PositionY) noexcept;

    VOID ResetDevice() noexcept;

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
