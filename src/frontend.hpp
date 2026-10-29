#pragma once

#include <xenklib/common.hpp>
#include <xenklib/xenbus.hpp>

// These descriptors are borrowed for the duration of mode enumeration.
struct VidPnCofuncConstraints {
    const D3DKMDT_VIDPN_PRESENT_PATH *Path;

    D3DKMDT_HVIDPNSOURCEMODESET SourceModeSet;
    const DXGK_VIDPNSOURCEMODESET_INTERFACE *SourceModeSetInterface;
    const D3DKMDT_VIDPN_SOURCE_MODE *PinnedSourceMode;
    bool SourceIsPivot;

    D3DKMDT_HVIDPNTARGETMODESET TargetModeSet;
    const DXGK_VIDPNTARGETMODESET_INTERFACE *TargetModeSetInterface;
    const D3DKMDT_VIDPN_TARGET_MODE *PinnedTargetMode;
    bool TargetIsPivot;
};

class VfbFrontend {
public:
    static constexpr size_t Bpp = 32;
    static constexpr size_t PixelBytes = Bpp / 8;

    explicit VfbFrontend(StoreHandle store, ULONG childUid);

    DECLSPEC_PAGED_CODE
    _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) static bool IsSizeValid(D3DKMDT_2DREGION size);

    DECLSPEC_PAGED_CODE _Check_return_
        _IRQL_requires_(PASSIVE_LEVEL) static bool IsPathSupported(const D3DKMDT_VIDPN_PRESENT_PATH &path);

    DECLSPEC_PAGED_CODE _Check_return_
        _IRQL_requires_(PASSIVE_LEVEL) static bool IsSourceModeSupported(const D3DKMDT_VIDPN_SOURCE_MODE &mode);

    DECLSPEC_PAGED_CODE _Check_return_
        _IRQL_requires_(PASSIVE_LEVEL) static bool IsTargetSignalSupported(const D3DKMDT_VIDEO_SIGNAL_INFO &signal);

    // Validates each supplied mode and requires matching visible/active sizes when both are supplied.
    // A null mode leaves that end unconstrained.
    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) static bool AreModesCompatible(
        _In_opt_ const D3DKMDT_VIDPN_SOURCE_MODE *sourceMode,
        _In_opt_ const D3DKMDT_VIDPN_TARGET_MODE *targetMode);

    // Updates unresolved transforms on a mutable path copy to support identity, preserving pivot flags.
    // Reports whether flags were written; returns unsupported if the resulting path cannot use identity.
    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) static NTSTATUS PrepareCofuncPath(
        _Inout_ D3DKMDT_VIDPN_PRESENT_PATH &path,
        _In_ const DXGKARG_ENUMVIDPNCOFUNCMODALITY &enumeration,
        _Out_ bool *supportFieldsModified);

    // Populates an unpinned, non-pivot source set from pinned/pivot target modes or SupportedResolutions.
    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) static NTSTATUS AddSourceModes(
        _In_ D3DKMDT_HVIDPNSOURCEMODESET modeSet,
        _In_ const DXGK_VIDPNSOURCEMODESET_INTERFACE *modeSetInterface,
        _In_ const VidPnCofuncConstraints &constraints);

    // Populates an unpinned, non-pivot target set from pinned/pivot source modes or SupportedResolutions.
    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) static NTSTATUS AddTargetModes(
        _In_ D3DKMDT_HVIDPNTARGETMODESET modeSet,
        _In_ const DXGK_VIDPNTARGETMODESET_INTERFACE *modeSetInterface,
        _In_ const VidPnCofuncConstraints &constraints);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        QueryDeviceDescriptor(INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        SetPowerState(IN_DEVICE_POWER_STATE DevicePowerState, IN_POWER_ACTION ActionType);

private:
    DECLSPEC_PAGED_CODE _Check_return_
        _IRQL_requires_(PASSIVE_LEVEL) static bool PathAllowsIdentity(const D3DKMDT_VIDPN_PRESENT_PATH &path);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) static NTSTATUS
        BuildTargetSignalInfo(_In_ const D3DKMDT_2DREGION &size, _Out_ D3DKMDT_VIDEO_SIGNAL_INFO *signal);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) static NTSTATUS AddSourceModeForSize(
        _In_ D3DKMDT_HVIDPNSOURCEMODESET modeSet,
        _In_ const DXGK_VIDPNSOURCEMODESET_INTERFACE *modeSetInterface,
        _In_ const D3DKMDT_2DREGION &size);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) static NTSTATUS AddTargetModeForSignal(
        _In_ D3DKMDT_HVIDPNTARGETMODESET modeSet,
        _In_ const DXGK_VIDPNTARGETMODESET_INTERFACE *modeSetInterface,
        _In_ const D3DKMDT_VIDEO_SIGNAL_INFO &signal);

    StoreHandle _store;
    BYTE _edid[128];
};
