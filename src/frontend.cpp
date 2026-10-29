#include "xenvfb.hpp"
#include "frontend.hpp"
#include "edid.hpp"

static const D3DKMDT_2DREGION SupportedResolutions[] = {
    {640, 480},   //
    {800, 480},   //
    {800, 600},   //
    {832, 624},   //
    {960, 640},   //
    {1024, 600},  //
    {1024, 768},  //
    {1152, 864},  //
    {1152, 870},  //
    {1280, 720},  //
    {1280, 760},  //
    {1280, 768},  //
    {1280, 800},  //
    {1280, 960},  //
    {1280, 1024}, //
    {1360, 768},  //
    {1366, 768},  //
    {1400, 1050}, //
    {1440, 900},  //
    {1440, 1080}, //
    {1600, 900},  //
    {1600, 1200}, //
    {1680, 1050}, //
    {1920, 1080}, //
    {1920, 1200}, //
    {1920, 1440}, //
    {2000, 2000}, //
    {2048, 1536}, //
    {2048, 2048}, //
    {2560, 1440}, //
    {2560, 1600}, //
    {2560, 2048}, //
    {2800, 2100}, //
    {3200, 2400}, //
    {3840, 2160}, //
    {4096, 2160}, //
    {7680, 4320}, //
    {8192, 4320}, //
};

// The callback borrows each descriptor for this invocation only. Return success
// to continue or an error to stop; this helper releases every acquired descriptor.
// Dataset-empty/end statuses are informational but supply no new descriptor.
template <typename TMode, typename TModeSet, typename TModeSetInterface, typename TCallback>
DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) static NTSTATUS
    ForEachMode(TModeSet modeSet, const TModeSetInterface *modeSetInterface, TCallback &&callback) {
    PAGED_CODE();

    const TMode *mode = nullptr;
    auto cleanupMode = wil::scope_exit([&]() {
        if (mode) {
            modeSetInterface->pfnReleaseModeInfo(modeSet, mode);
        }
    });

    bool firstMode = true;
    while (true) {
        const TMode *nextMode = nullptr;
        NTSTATUS status;

        if (firstMode) {
            status = modeSetInterface->pfnAcquireFirstModeInfo(modeSet, &nextMode);
        } else {
            status = modeSetInterface->pfnAcquireNextModeInfo(modeSet, mode, &nextMode);
        }
        if (status == STATUS_GRAPHICS_DATASET_IS_EMPTY || status == STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET) {
            return STATUS_SUCCESS;
        } else if (!NT_SUCCESS(status)) {
            return status;
        }

        if (firstMode) {
            mode = nextMode;
            firstMode = false;
        } else {
            modeSetInterface->pfnReleaseModeInfo(modeSet, wistd::exchange(mode, nextMode));
        }

        status = callback(*mode);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }
}

VfbFrontend::VfbFrontend(xenklib::StoreHandle store, ULONG childUid) : _store(wistd::move(store)) {
    VfbGetEdid(childUid, _edid);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) bool VfbFrontend::IsSizeValid(D3DKMDT_2DREGION size) {
    PAGED_CODE();

    if (size.cx < 640 || size.cy < 480 || static_cast<UINT64>(size.cx) * PixelBytes > MAXULONG) {
        return false;
    }
    // TODO: apply backend dimensions and framebuffer capacity once negotiated.
    return true;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontend::QueryDeviceDescriptor(INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) {
    PAGED_CODE();

    if (DeviceDescriptor->DescriptorOffset >= sizeof(_edid)) {
        return STATUS_MONITOR_NO_MORE_DESCRIPTOR_DATA;
    }

    RtlCopyMemory(
        DeviceDescriptor->DescriptorBuffer,
        _edid + DeviceDescriptor->DescriptorOffset,
        wistd::min(
            DeviceDescriptor->DescriptorLength,
            static_cast<ULONG>(sizeof(_edid)) - DeviceDescriptor->DescriptorOffset));
    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontend::SetPowerState(IN_DEVICE_POWER_STATE DevicePowerState, IN_POWER_ACTION ActionType) {
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DevicePowerState);
    UNREFERENCED_PARAMETER(ActionType);

    return STATUS_NOT_IMPLEMENTED;
}

DECLSPEC_PAGED_CODE _Check_return_
_IRQL_requires_(PASSIVE_LEVEL) bool VfbFrontend::IsPathSupported(const D3DKMDT_VIDPN_PRESENT_PATH &path) {
    PAGED_CODE();

    switch (path.ContentTransformation.Scaling) {
    case D3DKMDT_VPPS_IDENTITY:
    case D3DKMDT_VPPS_UNINITIALIZED:
    case D3DKMDT_VPPS_UNPINNED:
    case D3DKMDT_VPPS_NOTSPECIFIED:
        break;
    default:
        return false;
    }

    switch (path.ContentTransformation.Rotation) {
    case D3DKMDT_VPPR_IDENTITY:
    case D3DKMDT_VPPR_UNINITIALIZED:
    case D3DKMDT_VPPR_UNPINNED:
    case D3DKMDT_VPPR_NOTSPECIFIED:
        break;
    default:
        return false;
    }

    return (path.VidPnTargetColorBasis == D3DKMDT_CB_UNINITIALIZED || path.VidPnTargetColorBasis == D3DKMDT_CB_SCRGB) &&
        path.VisibleFromActiveTLOffset.cx == 0 && path.VisibleFromActiveTLOffset.cy == 0 &&
        path.VisibleFromActiveBROffset.cx == 0 && path.VisibleFromActiveBROffset.cy == 0 &&
        (path.CopyProtection.CopyProtectionType == D3DKMDT_VPPMT_UNINITIALIZED ||
         path.CopyProtection.CopyProtectionType == D3DKMDT_VPPMT_NOPROTECTION) &&
        (path.GammaRamp.Type == D3DDDI_GAMMARAMP_UNINITIALIZED || path.GammaRamp.Type == D3DDDI_GAMMARAMP_DEFAULT);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS VfbFrontend::PrepareCofuncPath(
    _Inout_ D3DKMDT_VIDPN_PRESENT_PATH &path,
    _In_ const DXGKARG_ENUMVIDPNCOFUNCMODALITY &enumeration,
    _Out_ bool *supportFieldsModified) {
    PAGED_CODE();

    *supportFieldsModified = false;
    if (!IsPathSupported(path)) {
        return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
    }

    auto pivotIsPath = enumeration.EnumPivot.VidPnSourceId == path.VidPnSourceId &&
        enumeration.EnumPivot.VidPnTargetId == path.VidPnTargetId;
    if (path.ContentTransformation.Scaling != D3DKMDT_VPPS_IDENTITY &&
        !(enumeration.EnumPivotType == D3DKMDT_EPT_SCALING && pivotIsPath)) {
        path.ContentTransformation.ScalingSupport = {.Identity = 1};
        *supportFieldsModified = true;
    }
    if (path.ContentTransformation.Rotation != D3DKMDT_VPPR_IDENTITY &&
        !(enumeration.EnumPivotType == D3DKMDT_EPT_ROTATION && pivotIsPath)) {
        path.ContentTransformation.RotationSupport = {.Identity = 1};
        *supportFieldsModified = true;
    }

    return PathAllowsIdentity(path) ? STATUS_SUCCESS : STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
}

// DDI requirement: modes must work with the path transformations. When a
// transformation is the pivot, its unchanged support flags can rule out identity.
DECLSPEC_PAGED_CODE _Check_return_
_IRQL_requires_(PASSIVE_LEVEL) bool VfbFrontend::PathAllowsIdentity(const D3DKMDT_VIDPN_PRESENT_PATH &path) {
    PAGED_CODE();

    switch (path.ContentTransformation.Scaling) {
    case D3DKMDT_VPPS_IDENTITY:
        break;
    case D3DKMDT_VPPS_UNINITIALIZED:
    case D3DKMDT_VPPS_UNPINNED:
    case D3DKMDT_VPPS_NOTSPECIFIED:
        if (!path.ContentTransformation.ScalingSupport.Identity) {
            return false;
        }
        break;
    default:
        return false;
    }

    switch (path.ContentTransformation.Rotation) {
    case D3DKMDT_VPPR_IDENTITY:
        return true;
    case D3DKMDT_VPPR_UNINITIALIZED:
    case D3DKMDT_VPPR_UNPINNED:
    case D3DKMDT_VPPR_NOTSPECIFIED:
        return path.ContentTransformation.RotationSupport.Identity != 0;
    default:
        return false;
    }
}

// A preserved pivot may contain modes that our framebuffer cannot use. Matching
// its dimensions alone is insufficient: the matching source must also be usable.
DECLSPEC_PAGED_CODE _Check_return_
_IRQL_requires_(PASSIVE_LEVEL) bool VfbFrontend::IsSourceModeSupported(const D3DKMDT_VIDPN_SOURCE_MODE &mode) {
    PAGED_CODE();

    if (mode.Type != D3DKMDT_RMT_GRAPHICS) {
        return false;
    }

    const auto &graphics = mode.Format.Graphics;
    return IsSizeValid(graphics.VisibleRegionSize) && graphics.PrimSurfSize.cx == graphics.VisibleRegionSize.cx &&
        graphics.PrimSurfSize.cy == graphics.VisibleRegionSize.cy &&
        graphics.Stride >= static_cast<UINT64>(graphics.PrimSurfSize.cx) * PixelBytes &&
        graphics.PixelFormat == D3DDDIFMT_A8R8G8B8 &&
        (graphics.ColorBasis == D3DKMDT_CB_UNINITIALIZED || graphics.ColorBasis == D3DKMDT_CB_SCRGB) &&
        graphics.PixelValueAccessMode == D3DKMDT_PVAM_DIRECT;
}

DECLSPEC_PAGED_CODE _Check_return_
_IRQL_requires_(PASSIVE_LEVEL) bool VfbFrontend::IsTargetSignalSupported(const D3DKMDT_VIDEO_SIGNAL_INFO &signal) {
    PAGED_CODE();

    return IsSizeValid(signal.ActiveSize) && signal.TotalSize.cx == signal.ActiveSize.cx &&
        signal.TotalSize.cy == signal.ActiveSize.cy;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) bool VfbFrontend::AreModesCompatible(
    _In_opt_ const D3DKMDT_VIDPN_SOURCE_MODE *sourceMode,
    _In_opt_ const D3DKMDT_VIDPN_TARGET_MODE *targetMode) {
    PAGED_CODE();

    if (sourceMode && !IsSourceModeSupported(*sourceMode)) {
        return false;
    }
    if (targetMode && !IsTargetSignalSupported(targetMode->VideoSignalInfo)) {
        return false;
    }
    if (sourceMode && targetMode) {
        const auto &sourceSize = sourceMode->Format.Graphics.VisibleRegionSize;
        const auto &targetSize = targetMode->VideoSignalInfo.ActiveSize;
        return sourceSize.cx == targetSize.cx && sourceSize.cy == targetSize.cy;
    }
    return true;
}

// DDI descriptor procedure: create a descriptor, keep its generated Id, populate
// the format, then AddMode. A successful AddMode releases the descriptor; failed
// additions leave it to our guard. An existing identical mode also counts as success.
// Candidate selection and framebuffer checks belong to the calling mode builder.
DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS VfbFrontend::AddSourceModeForSize(
    _In_ D3DKMDT_HVIDPNSOURCEMODESET modeSet,
    _In_ const DXGK_VIDPNSOURCEMODESET_INTERFACE *modeSetInterface,
    _In_ const D3DKMDT_2DREGION &size) {
    PAGED_CODE();

    NTSTATUS status;

    if (!IsSizeValid(size)) {
        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }

    D3DKMDT_VIDPN_SOURCE_MODE *mode = nullptr;
    status = modeSetInterface->pfnCreateNewModeInfo(modeSet, &mode);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    auto cleanupMode = wil::scope_exit([&]() {
        if (mode) {
            modeSetInterface->pfnReleaseModeInfo(modeSet, mode);
        }
    });

    mode->Type = D3DKMDT_RMT_GRAPHICS;
    mode->Format.Graphics = {};
    auto &graphics = mode->Format.Graphics;
    graphics.PrimSurfSize = size;
    graphics.VisibleRegionSize = size;
    graphics.Stride = static_cast<UINT>(size.cx * PixelBytes);
    graphics.PixelFormat = D3DDDIFMT_A8R8G8B8;
    graphics.ColorBasis = D3DKMDT_CB_SCRGB;
    graphics.PixelValueAccessMode = D3DKMDT_PVAM_DIRECT;

    status = modeSetInterface->pfnAddMode(modeSet, mode);
    if (status == STATUS_GRAPHICS_MODE_ALREADY_IN_MODESET) {
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }

    mode = nullptr;
    return STATUS_SUCCESS;
}

// Use the same ownership procedure for target descriptors. The caller constructs
// the supported signal before this helper adds it to the replacement set.
DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS VfbFrontend::AddTargetModeForSignal(
    _In_ D3DKMDT_HVIDPNTARGETMODESET modeSet,
    _In_ const DXGK_VIDPNTARGETMODESET_INTERFACE *modeSetInterface,
    _In_ const D3DKMDT_VIDEO_SIGNAL_INFO &signal) {
    PAGED_CODE();

    NTSTATUS status;

    if (!IsTargetSignalSupported(signal)) {
        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET_MODE;
    }

    D3DKMDT_VIDPN_TARGET_MODE *mode = nullptr;
    status = modeSetInterface->pfnCreateNewModeInfo(modeSet, &mode);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    auto cleanupMode = wil::scope_exit([&]() {
        if (mode) {
            modeSetInterface->pfnReleaseModeInfo(modeSet, mode);
        }
    });

    auto id = mode->Id;
    *mode = {};
    mode->Id = id;
    mode->VideoSignalInfo = signal;
    mode->Preference = D3DKMDT_MP_NOTPREFERRED;

    status = modeSetInterface->pfnAddMode(modeSet, mode);
    if (status == STATUS_GRAPHICS_MODE_ALREADY_IN_MODESET) {
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }

    mode = nullptr;
    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontend::BuildTargetSignalInfo(_In_ const D3DKMDT_2DREGION &size, _Out_ D3DKMDT_VIDEO_SIGNAL_INFO *signal) {
    PAGED_CODE();

    *signal = {};

    if (!IsSizeValid(size)) {
        return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
    }

    // Virtual output has no physical blanking or refresh clock (as in xstdvga).
    signal->VideoStandard = D3DKMDT_VSS_OTHER;
    signal->ActiveSize = size;
    signal->TotalSize = size;
    signal->VSyncFreq = {D3DKMDT_FREQUENCY_NOTSPECIFIED, D3DKMDT_FREQUENCY_NOTSPECIFIED};
    signal->HSyncFreq = {D3DKMDT_FREQUENCY_NOTSPECIFIED, D3DKMDT_FREQUENCY_NOTSPECIFIED};
    signal->PixelRate = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    signal->ScanLineOrdering = D3DDDI_VSSLO_PROGRESSIVE;
    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontend::AddSourceModes(
    _In_ D3DKMDT_HVIDPNSOURCEMODESET newSourceModeSet,
    _In_ const DXGK_VIDPNSOURCEMODESET_INTERFACE *newSourceModeSetInterface,
    _In_ const VidPnCofuncConstraints &constraints) {
    PAGED_CODE();

    ASSERT(!constraints.PinnedSourceMode && !constraints.SourceIsPivot);
    const auto &path = *constraints.Path;
    if (!IsPathSupported(path) || !PathAllowsIdentity(path)) {
        return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
    }

    bool foundSupportedMode = false;

    auto addSize = [&](const D3DKMDT_2DREGION &size) -> NTSTATUS {
        // Reject unusable candidates without stopping enumeration of other sizes.
        // The stride calculation must also fit the descriptor's UINT field.
        if (!IsSizeValid(size)) {
            return STATUS_SUCCESS;
        }

        auto status = AddSourceModeForSize(newSourceModeSet, newSourceModeSetInterface, size);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        foundSupportedMode = true;
        return STATUS_SUCCESS;
    };

    if (constraints.PinnedTargetMode) {
        // DDI requirement: preserve the target's pinned mode. With identity
        // presentation, the new source must have exactly that target's active size.
        const auto &signal = constraints.PinnedTargetMode->VideoSignalInfo;
        if (!IsTargetSignalSupported(signal)) {
            return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
        }
        auto status = addSize(signal.ActiveSize);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    } else if (constraints.TargetIsPivot) {
        // DDI pivot rule: the target set stays unchanged. Enumerate its actual
        // modes and add a source for every usable target size in that preserved set.
        auto status = ForEachMode<D3DKMDT_VIDPN_TARGET_MODE>(
            constraints.TargetModeSet,
            constraints.TargetModeSetInterface,
            [&](const D3DKMDT_VIDPN_TARGET_MODE &mode) -> NTSTATUS {
                if (!IsTargetSignalSupported(mode.VideoSignalInfo)) {
                    return STATUS_SUCCESS;
                }
                return addSize(mode.VideoSignalInfo.ActiveSize);
            });
        if (!NT_SUCCESS(status)) {
            return status;
        }
    } else {
        // Neither end fixes the target choices. Use our supported resolution
        // policy; the existing unpinned, non-pivot target set may be incomplete.
        for (const auto &size : SupportedResolutions) {
            // Every published source needs a supported target counterpart. Use
            // the same signal builder as AddTargetModes before publishing the size.
            D3DKMDT_VIDEO_SIGNAL_INFO signal = {};
            auto status = BuildTargetSignalInfo(size, &signal);
            if (status == STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED) {
                continue;
            } else if (!NT_SUCCESS(status)) {
                return status;
            }
            if (!IsTargetSignalSupported(signal) || signal.ActiveSize.cx != size.cx ||
                signal.ActiveSize.cy != size.cy) {
                return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET_MODE;
            }
            status = addSize(size);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
    }

    // Do not claim success when none of the candidates could produce a usable
    // source. AssignSourceModeSet cannot publish an empty replacement set.
    return foundSupportedMode ? STATUS_SUCCESS : STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontend::AddTargetModes(
    _In_ D3DKMDT_HVIDPNTARGETMODESET newTargetModeSet,
    _In_ const DXGK_VIDPNTARGETMODESET_INTERFACE *newTargetModeSetInterface,
    _In_ const VidPnCofuncConstraints &constraints) {
    PAGED_CODE();

    ASSERT(!constraints.PinnedTargetMode && !constraints.TargetIsPivot);
    const auto &path = *constraints.Path;
    if (!IsPathSupported(path) || !PathAllowsIdentity(path)) {
        return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
    }

    bool foundSupportedMode = false;

    auto addSize = [&](const D3DKMDT_2DREGION &size) -> NTSTATUS {
        // Apply the same size/stride limits as the source builder so a target
        // generated from our resolution policy can have a usable matching source.
        if (!IsSizeValid(size)) {
            return STATUS_SUCCESS;
        }

        D3DKMDT_VIDEO_SIGNAL_INFO signal = {};
        auto status = BuildTargetSignalInfo(size, &signal);
        if (status == STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED) {
            return STATUS_SUCCESS;
        } else if (!NT_SUCCESS(status)) {
            return status;
        }
        if (!IsTargetSignalSupported(signal) || signal.ActiveSize.cx != size.cx || signal.ActiveSize.cy != size.cy) {
            return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET_MODE;
        }

        status = AddTargetModeForSignal(newTargetModeSet, newTargetModeSetInterface, signal);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        foundSupportedMode = true;
        return STATUS_SUCCESS;
    };

    if (constraints.PinnedSourceMode) {
        // DDI requirement: preserve the source's pinned mode. Identity requires
        // a target signal with exactly the source's visible dimensions.
        if (!IsSourceModeSupported(*constraints.PinnedSourceMode)) {
            return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
        }
        auto status = addSize(constraints.PinnedSourceMode->Format.Graphics.VisibleRegionSize);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    } else if (constraints.SourceIsPivot) {
        // DDI pivot rule: preserve the source set and add target modes that match
        // usable source modes actually present in that set.
        auto status = ForEachMode<D3DKMDT_VIDPN_SOURCE_MODE>(
            constraints.SourceModeSet,
            constraints.SourceModeSetInterface,
            [&](const D3DKMDT_VIDPN_SOURCE_MODE &mode) -> NTSTATUS {
                if (!IsSourceModeSupported(mode)) {
                    return STATUS_SUCCESS;
                }
                return addSize(mode.Format.Graphics.VisibleRegionSize);
            });
        if (!NT_SUCCESS(status)) {
            return status;
        }
    } else {
        // Both ends can be rebuilt. Use the SAME policy as AddSourceModes so
        // each target has a matching source, without intersecting incomplete old sets.
        for (const auto &size : SupportedResolutions) {
            auto status = addSize(size);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
    }

    return foundSupportedMode ? STATUS_SUCCESS : STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
}
