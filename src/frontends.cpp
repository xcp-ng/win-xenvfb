#include "common.hpp"
#include "xenbus.hpp"
#include "dbg_print.hpp"
#include "frontends.hpp"

NTSTATUS VfbFrontendManager::Initialize() noexcept {
    NTSTATUS status;

    status = _store(_store->StoreWatchAdd, const_cast<PSTR>("device"), const_cast<PSTR>("vfb"), &_threadEvent, &_watch);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    auto cleanupWatch = wil::scope_exit([&]() { _store(_store->StoreWatchRemove, _watch); });

    status = ThreadStart();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    cleanupWatch.release();
    return STATUS_SUCCESS;
}

NTSTATUS VfbFrontendManager::Enumerate() noexcept {
    NTSTATUS status;

    StoreString paths = _store.EmptyString();
    status = _store.Directory(paths, "device", "vfb");
    // STATUS_OBJECT_PATH_NOT_FOUND == "this is a leaf"
    // STATUS_OBJECT_NAME_NOT_FOUND == "this does not exist"
    if (status == STATUS_OBJECT_PATH_NOT_FOUND || status == STATUS_OBJECT_NAME_NOT_FOUND) {
    } else if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_NOT_IMPLEMENTED;
}

void VfbFrontendManager::ThreadWork() noexcept {
    while (true) {
        for (;;) {
            Trace("waiting...\n");

            KeWaitForSingleObject(&_threadEvent, Executive, KernelMode, FALSE, NULL);
            KeClearEvent(&_threadEvent);

            if (ThreadIsAlerted()) {
                break;
            }

            Enumerate();
        }
    }
}

VfbFrontendManager::~VfbFrontendManager() {
    ThreadAlert();
    ThreadJoin();
    if (_watch) {
        _store(_store->StoreWatchRemove, _watch);
    }
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontendManager::QueryDeviceDescriptor(IN_ULONG ChildUid, INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) noexcept {
    auto lock = _lock.AcquireShared();

    if (ChildUid >= MaxChildren) {
        return STATUS_GRAPHICS_CHILD_DESCRIPTOR_NOT_SUPPORTED;
    }

    return _frontends[ChildUid]->QueryDeviceDescriptor(DeviceDescriptor);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontendManager::SetPowerState(
    IN_ULONG DeviceUid,
    IN_DEVICE_POWER_STATE DevicePowerState,
    IN_POWER_ACTION ActionType) noexcept {
    auto lock = _lock.AcquireShared();

    if (DeviceUid >= MaxChildren) {
        return STATUS_SUCCESS;
    }

    return _frontends[DeviceUid]->SetPowerState(DevicePowerState, ActionType);
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontendManager::IsSupportedVidPn(
    INOUT_PDXGKARG_ISSUPPORTEDVIDPN pIsSupportedVidPn,
    _In_ const DXGK_VIDPN_INTERFACE *vidPnInterface,
    _In_ D3DKMDT_HVIDPNTOPOLOGY vidPnTopology,
    _In_ const DXGK_VIDPNTOPOLOGY_INTERFACE *vidPnTopologyInterface) noexcept {
    NTSTATUS status;

    pIsSupportedVidPn->IsVidPnSupported = FALSE;

    SIZE_T numPaths;
    status = vidPnTopologyInterface->pfnGetNumPaths(vidPnTopology, &numPaths);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (numPaths == 0) {
        pIsSupportedVidPn->IsVidPnSupported = TRUE;
        return STATUS_SUCCESS;
    }

    bool encountered[MaxViews][MaxChildren] = {};
    {
        const D3DKMDT_VIDPN_PRESENT_PATH *path = nullptr;
        auto cleanupPath = wil::scope_exit([&]() {
            if (path) {
                vidPnTopologyInterface->pfnReleasePathInfo(vidPnTopology, path);
            }
        });

        bool firstPath = true;
        while (true) {
            const D3DKMDT_VIDPN_PRESENT_PATH *nextPath = nullptr;
            if (firstPath) {
                status = vidPnTopologyInterface->pfnAcquireFirstPathInfo(vidPnTopology, &nextPath);
            } else {
                status = vidPnTopologyInterface->pfnAcquireNextPathInfo(vidPnTopology, path, &nextPath);
            }

            // These are informational statuses, not errors. But if Acquire*PathInfo returns these values, then
            // nextPath is not valid and cannot be used.
            if (status == STATUS_GRAPHICS_DATASET_IS_EMPTY || status == STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET) {
                break;
            } else if (!NT_SUCCESS(status)) {
                return status;
            }

            if (firstPath) {
                path = nextPath;
                firstPath = false;
            } else {
                vidPnTopologyInterface->pfnReleasePathInfo(vidPnTopology, wistd::exchange(path, nextPath));
            }

            if (path->VidPnSourceId >= MaxViews || path->VidPnTargetId >= MaxChildren) {
                return STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY;
            }

            if (path->VidPnSourceId != path->VidPnTargetId) {
                return STATUS_SUCCESS;
            }

            if (path->ContentTransformation.Scaling != D3DKMDT_VPPS_UNINITIALIZED &&
                path->ContentTransformation.Scaling != D3DKMDT_VPPS_IDENTITY &&
                path->ContentTransformation.Scaling != D3DKMDT_VPPS_UNPINNED &&
                path->ContentTransformation.Scaling != D3DKMDT_VPPS_NOTSPECIFIED) {
                // resolution correspondence is checked in the mode checks
                return STATUS_SUCCESS;
            }
            if (path->ContentTransformation.Rotation != D3DKMDT_VPPR_UNINITIALIZED &&
                path->ContentTransformation.Rotation != D3DKMDT_VPPR_IDENTITY &&
                path->ContentTransformation.Rotation != D3DKMDT_VPPR_UNPINNED &&
                path->ContentTransformation.Rotation != D3DKMDT_VPPR_NOTSPECIFIED) {
                return STATUS_SUCCESS;
            }
            if (path->VidPnTargetColorBasis != D3DKMDT_CB_UNINITIALIZED &&
                path->VidPnTargetColorBasis != D3DKMDT_CB_SCRGB) {
                return STATUS_SUCCESS;
            }
            if (path->VisibleFromActiveTLOffset.cx != 0 || path->VisibleFromActiveTLOffset.cy != 0) {
                return STATUS_SUCCESS;
            }
            if (path->VisibleFromActiveBROffset.cx != 0 || path->VisibleFromActiveBROffset.cy != 0) {
                return STATUS_SUCCESS;
            }
            if (path->CopyProtection.CopyProtectionType != D3DKMDT_VPPMT_UNINITIALIZED &&
                path->CopyProtection.CopyProtectionType != D3DKMDT_VPPMT_NOPROTECTION) {
                return STATUS_SUCCESS;
            }
            if (path->GammaRamp.Type != D3DDDI_GAMMARAMP_UNINITIALIZED &&
                path->GammaRamp.Type != D3DDDI_GAMMARAMP_DEFAULT) {
                return STATUS_SUCCESS;
            }

            if (wistd::exchange(encountered[path->VidPnSourceId][path->VidPnTargetId], true)) {
                return STATUS_SUCCESS;
            }
        }
    }

    auto lock = _lock.AcquireShared();

    D3DKMDT_2DREGION sourceSizes[MaxViews] = {};
    for (auto sourceId = 0; sourceId < MaxViews; sourceId++) {
        bool sourceIsActive = false;
        for (size_t i = 0; i < MaxChildren; i++) {
            if (encountered[sourceId][i]) {
                sourceIsActive = true;
                break;
            }
        }
        if (!sourceIsActive) {
            continue;
        }

        D3DKMDT_HVIDPNSOURCEMODESET sourceModeSet;
        const DXGK_VIDPNSOURCEMODESET_INTERFACE *sourceModeSetInterface;
        status = vidPnInterface->pfnAcquireSourceModeSet(
            pIsSupportedVidPn->hDesiredVidPn,
            sourceId,
            &sourceModeSet,
            &sourceModeSetInterface);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        auto cleanupSourceModeSet = wil::scope_exit(
            [&]() { vidPnInterface->pfnReleaseSourceModeSet(pIsSupportedVidPn->hDesiredVidPn, sourceModeSet); });

        const D3DKMDT_VIDPN_SOURCE_MODE *pinnedSourceMode;
        status = sourceModeSetInterface->pfnAcquirePinnedModeInfo(sourceModeSet, &pinnedSourceMode);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        if (!pinnedSourceMode) {
            continue;
        }
        auto cleanupPinnedSourceMode =
            wil::scope_exit([&]() { sourceModeSetInterface->pfnReleaseModeInfo(sourceModeSet, pinnedSourceMode); });

        if (pinnedSourceMode->Type != D3DKMDT_RMT_GRAPHICS) {
            return STATUS_SUCCESS;
        }

        const auto &graphics = pinnedSourceMode->Format.Graphics;
        if (graphics.PrimSurfSize.cx != graphics.VisibleRegionSize.cx ||
            graphics.PrimSurfSize.cy != graphics.VisibleRegionSize.cy) {
            return STATUS_SUCCESS;
        }
        if (graphics.Stride < static_cast<UINT64>(graphics.PrimSurfSize.cx) * VfbFrontend::PixelBytes) {
            return STATUS_SUCCESS;
        }
        // KMDOD
        if (graphics.PixelFormat != D3DDDIFMT_A8R8G8B8) {
            return STATUS_SUCCESS;
        }
        if (graphics.ColorBasis != D3DKMDT_CB_UNINITIALIZED && graphics.ColorBasis != D3DKMDT_CB_SCRGB) {
            return STATUS_SUCCESS;
        }
        if (graphics.PixelValueAccessMode != D3DKMDT_PVAM_DIRECT) {
            return STATUS_SUCCESS;
        }

        const auto &sourceFrontend = _frontends[sourceId];
        if (sourceFrontend && !sourceFrontend->IsSizeValid(graphics.VisibleRegionSize)) {
            return STATUS_SUCCESS;
        }
        sourceSizes[sourceId] = graphics.VisibleRegionSize;
    }

    for (auto targetId = 0; targetId < MaxChildren; targetId++) {
        bool targetIsActive = false;
        for (size_t i = 0; i < MaxViews; i++) {
            if (encountered[i][targetId]) {
                targetIsActive = true;
                break;
            }
        }
        if (!targetIsActive) {
            continue;
        }

        D3DKMDT_HVIDPNTARGETMODESET targetModeSet;
        const DXGK_VIDPNTARGETMODESET_INTERFACE *targetModeSetInterface;
        status = vidPnInterface->pfnAcquireTargetModeSet(
            pIsSupportedVidPn->hDesiredVidPn,
            targetId,
            &targetModeSet,
            &targetModeSetInterface);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        auto cleanupTargetModeSet = wil::scope_exit(
            [&]() { vidPnInterface->pfnReleaseTargetModeSet(pIsSupportedVidPn->hDesiredVidPn, targetModeSet); });

        const D3DKMDT_VIDPN_TARGET_MODE *pinnedTargetMode;
        status = targetModeSetInterface->pfnAcquirePinnedModeInfo(targetModeSet, &pinnedTargetMode);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (!pinnedTargetMode) {
            continue;
        }
        auto cleanupPinnedTargetMode =
            wil::scope_exit([&]() { targetModeSetInterface->pfnReleaseModeInfo(targetModeSet, pinnedTargetMode); });

        const auto &vsi = pinnedTargetMode->VideoSignalInfo;
        if (vsi.TotalSize.cx != vsi.ActiveSize.cx || vsi.TotalSize.cy != vsi.ActiveSize.cy) {
            return STATUS_SUCCESS;
        }

        // source ID must == target ID per above check
        if (sourceSizes[targetId].cx != 0 && sourceSizes[targetId].cy != 0) {
            if (vsi.ActiveSize.cx != sourceSizes[targetId].cx || vsi.ActiveSize.cy != sourceSizes[targetId].cy) {
                return STATUS_SUCCESS;
            }
        } else {
            const auto &targetFrontend = _frontends[targetId];
            if (targetFrontend && !targetFrontend->IsSizeValid(vsi.ActiveSize)) {
                return STATUS_SUCCESS;
            }
        }
    }

    pIsSupportedVidPn->IsVidPnSupported = TRUE;
    return STATUS_SUCCESS;
}

struct VidPnCofuncConstraints {
    const DXGKARG_ENUMVIDPNCOFUNCMODALITY *Enumeration;
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

static NTSTATUS AddSourceModes(
    _In_ D3DKMDT_HVIDPNSOURCEMODESET newSourceModeSet,
    _In_ const DXGK_VIDPNSOURCEMODESET_INTERFACE *newSourceModeSetInterface,
    _In_ const VidPnCofuncConstraints &constraints) noexcept;

static NTSTATUS AddTargetModes(
    _In_ D3DKMDT_HVIDPNTARGETMODESET newTargetModeSet,
    _In_ const DXGK_VIDPNTARGETMODESET_INTERFACE *newTargetModeSetInterface,
    _In_ const VidPnCofuncConstraints &constraints) noexcept;

// Preserve pinned and pivot mode sets. Rebuild the other sets with all supported modes that can work with the preserved
// modes and path settings.
//
// The DDI requires us to:
// - Inspect the topology and existing modes to find those constraints.
// - Preserve pinned mode sets and any mode set designated as the pivot.
// - Make each unpinned, non-pivot mode set the largest possible set compatible with the constraints.
// - Update unpinned scaling/rotation support, except for a transformation pivot.
//
// This function implements the inspection, preservation and publication steps. AddSourceModes/AddTargetModes implement
// the actual compatible-mode enumeration.
DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontendManager::EnumVidPnCofuncModality(
    IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality,
    _In_ const DXGK_VIDPN_INTERFACE *vidPnInterface,
    _In_ D3DKMDT_HVIDPNTOPOLOGY vidPnTopology,
    _In_ const DXGK_VIDPNTOPOLOGY_INTERFACE *vidPnTopologyInterface) noexcept {
    NTSTATUS status;

    // Keep framebuffer limits and mode policy stable while building both sets.
    auto lock = _lock.AcquireShared();

    auto vidPn = pEnumCofuncModality->hConstrainingVidPn;
    bool encountered[MaxViews][MaxChildren] = {};

    const D3DKMDT_VIDPN_PRESENT_PATH *path = nullptr;
    auto cleanupPath = wil::scope_exit([&]() {
        if (path) {
            vidPnTopologyInterface->pfnReleasePathInfo(vidPnTopology, path);
        }
    });

    // DDI requirement: inspect the constraining topology first.
    bool firstPath = true;
    while (true) {
        const D3DKMDT_VIDPN_PRESENT_PATH *nextPath = nullptr;
        if (firstPath) {
            status = vidPnTopologyInterface->pfnAcquireFirstPathInfo(vidPnTopology, &nextPath);
        } else {
            status = vidPnTopologyInterface->pfnAcquireNextPathInfo(vidPnTopology, path, &nextPath);
        }

        if (status == STATUS_GRAPHICS_DATASET_IS_EMPTY || status == STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET) {
            break;
        } else if (!NT_SUCCESS(status)) {
            return status;
        }

        if (firstPath) {
            path = nextPath;
            firstPath = false;
        } else {
            vidPnTopologyInterface->pfnReleasePathInfo(vidPnTopology, wistd::exchange(path, nextPath));
        }

        // The one-to-one source-target correspondence is **our** requirement rather than DDI requirements.
        auto sourceId = path->VidPnSourceId;
        auto targetId = path->VidPnTargetId;
        if (sourceId >= MaxViews || targetId >= MaxChildren) {
            return STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY;
        }
        if (sourceId != targetId) {
            return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
        }
        if (wistd::exchange(encountered[sourceId][targetId], true)) {
            return STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY;
        }

        // DDI pivot rule: a source/target pivot keeps its whole mode set unchanged, even if unpinned.
        // A scaling/rotation pivot keeps the corresponding support flags unchanged.
        auto sourceIsPivot = pEnumCofuncModality->EnumPivotType == D3DKMDT_EPT_VIDPNSOURCE &&
            pEnumCofuncModality->EnumPivot.VidPnSourceId == sourceId;
        auto targetIsPivot = pEnumCofuncModality->EnumPivotType == D3DKMDT_EPT_VIDPNTARGET &&
            pEnumCofuncModality->EnumPivot.VidPnTargetId == targetId;
        auto scalingIsPivot = pEnumCofuncModality->EnumPivotType == D3DKMDT_EPT_SCALING &&
            pEnumCofuncModality->EnumPivot.VidPnSourceId == sourceId &&
            pEnumCofuncModality->EnumPivot.VidPnTargetId == targetId;
        auto rotationIsPivot = pEnumCofuncModality->EnumPivotType == D3DKMDT_EPT_ROTATION &&
            pEnumCofuncModality->EnumPivot.VidPnSourceId == sourceId &&
            pEnumCofuncModality->EnumPivot.VidPnTargetId == targetId;

        // DDI requirement: update support flags for unpinned, non-pivot transformations.
        // Modify a copy because the acquired path descriptor is read-only.
        auto localPath = *path;
        bool supportFieldsModified = false;

        // Keep concrete transformations fixed. For unresolved values, advertise identity unless the corresponding
        // transformation is the pivot.
        switch (path->ContentTransformation.Scaling) {
        case D3DKMDT_VPPS_IDENTITY:
            break;
        case D3DKMDT_VPPS_UNINITIALIZED:
        case D3DKMDT_VPPS_UNPINNED:
        case D3DKMDT_VPPS_NOTSPECIFIED:
            if (!scalingIsPivot) {
                localPath.ContentTransformation.ScalingSupport = {.Identity = 1};
                supportFieldsModified = true;
            }
            break;
        default:
            return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
        }

        switch (path->ContentTransformation.Rotation) {
        case D3DKMDT_VPPR_IDENTITY:
            break;
        case D3DKMDT_VPPR_UNINITIALIZED:
        case D3DKMDT_VPPR_UNPINNED:
        case D3DKMDT_VPPR_NOTSPECIFIED:
            if (!rotationIsPivot) {
                localPath.ContentTransformation.RotationSupport = {.Identity = 1};
                supportFieldsModified = true;
            }
            break;
        default:
            return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
        }

        // VidPN replacement procedure: create new sets, populate them, then replace the old sets.
        D3DKMDT_HVIDPNSOURCEMODESET newSourceModeSet = nullptr;
        auto cleanupNewSourceModeSet = wil::scope_exit([&]() {
            if (newSourceModeSet) {
                vidPnInterface->pfnReleaseSourceModeSet(vidPn, newSourceModeSet);
            }
        });
        D3DKMDT_HVIDPNTARGETMODESET newTargetModeSet = nullptr;
        auto cleanupNewTargetModeSet = wil::scope_exit([&]() {
            if (newTargetModeSet) {
                vidPnInterface->pfnReleaseTargetModeSet(vidPn, newTargetModeSet);
            }
        });

        // DDI inspection requirement: inspect pinned modes on both ends before building replacements.
        // A pinned target restricts source modes, a pinned source restricts target modes.
        // This scope releases the old mode-set references before assigning replacements.
        {
            D3DKMDT_HVIDPNSOURCEMODESET sourceModeSet = nullptr;
            const DXGK_VIDPNSOURCEMODESET_INTERFACE *sourceModeSetInterface = nullptr;
            status = vidPnInterface->pfnAcquireSourceModeSet(vidPn, sourceId, &sourceModeSet, &sourceModeSetInterface);
            if (!NT_SUCCESS(status)) {
                return status;
            }
            auto cleanupSourceModeSet =
                wil::scope_exit([&]() { vidPnInterface->pfnReleaseSourceModeSet(vidPn, sourceModeSet); });

            D3DKMDT_HVIDPNTARGETMODESET targetModeSet = nullptr;
            const DXGK_VIDPNTARGETMODESET_INTERFACE *targetModeSetInterface = nullptr;
            status = vidPnInterface->pfnAcquireTargetModeSet(vidPn, targetId, &targetModeSet, &targetModeSetInterface);
            if (!NT_SUCCESS(status)) {
                return status;
            }
            auto cleanupTargetModeSet =
                wil::scope_exit([&]() { vidPnInterface->pfnReleaseTargetModeSet(vidPn, targetModeSet); });

            const D3DKMDT_VIDPN_SOURCE_MODE *pinnedSourceMode = nullptr;
            status = sourceModeSetInterface->pfnAcquirePinnedModeInfo(sourceModeSet, &pinnedSourceMode);
            if (!NT_SUCCESS(status)) {
                return status;
            }
            auto cleanupPinnedSourceMode = wil::scope_exit([&]() {
                if (pinnedSourceMode) {
                    sourceModeSetInterface->pfnReleaseModeInfo(sourceModeSet, pinnedSourceMode);
                }
            });

            const D3DKMDT_VIDPN_TARGET_MODE *pinnedTargetMode = nullptr;
            status = targetModeSetInterface->pfnAcquirePinnedModeInfo(targetModeSet, &pinnedTargetMode);
            if (!NT_SUCCESS(status)) {
                return status;
            }
            auto cleanupPinnedTargetMode = wil::scope_exit([&]() {
                if (pinnedTargetMode) {
                    targetModeSetInterface->pfnReleaseModeInfo(targetModeSet, pinnedTargetMode);
                }
            });

            // With our identity presentation, different pinned sizes cannot work together.
            // Enumeration cannot change either pinned mode to fix a mismatch.
            if (pinnedSourceMode && pinnedSourceMode->Type != D3DKMDT_RMT_GRAPHICS) {
                return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
            }
            if (pinnedSourceMode && pinnedTargetMode) {
                const auto &sourceSize = pinnedSourceMode->Format.Graphics.VisibleRegionSize;
                const auto &targetSize = pinnedTargetMode->VideoSignalInfo.ActiveSize;
                if (sourceSize.cx != targetSize.cx || sourceSize.cy != targetSize.cy) {
                    return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
                }
            }

            // Besides resolution, the DDI constraints include path transformations, target color basis/ranges, etc.
            // Pass them all to AddSourceModes/AddTargetModes.
            // constraints is valid in this scope only.
            const VidPnCofuncConstraints constraints = {
                pEnumCofuncModality,
                &localPath,
                sourceModeSet,
                sourceModeSetInterface,
                pinnedSourceMode,
                sourceIsPivot,
                targetModeSet,
                targetModeSetInterface,
                pinnedTargetMode,
                targetIsPivot,
            };

            // DDI mode-set preservation rule: Leave pinned and pivot sets unchanged.
            // Otherwise, include every supported source mode that can match the pinned target or a mode in the
            // preserved target pivot set.
            // If neither restricts target choices, use our supported resolution policy.
            // Existing unpinned, non-pivot sets may be incomplete; they can be replaced.
            if (!pinnedSourceMode && !sourceIsPivot) {
                const DXGK_VIDPNSOURCEMODESET_INTERFACE *newSourceModeSetInterface = nullptr;
                status = vidPnInterface->pfnCreateNewSourceModeSet(
                    vidPn,
                    sourceId,
                    &newSourceModeSet,
                    &newSourceModeSetInterface);
                if (!NT_SUCCESS(status)) {
                    return status;
                }
                status = AddSourceModes(newSourceModeSet, newSourceModeSetInterface, constraints);
                if (!NT_SUCCESS(status)) {
                    return status;
                }
            }

            // Build target modes under the corresponding source constraints.
            // If both sets are rebuilt, every mode needs a supported match in the other set.
            if (!pinnedTargetMode && !targetIsPivot) {
                const DXGK_VIDPNTARGETMODESET_INTERFACE *newTargetModeSetInterface = nullptr;
                status = vidPnInterface->pfnCreateNewTargetModeSet(
                    vidPn,
                    targetId,
                    &newTargetModeSet,
                    &newTargetModeSetInterface);
                if (!NT_SUCCESS(status)) {
                    return status;
                }
                status = AddTargetModes(newTargetModeSet, newTargetModeSetInterface, constraints);
                if (!NT_SUCCESS(status)) {
                    return status;
                }
            }
        }

        if (newSourceModeSet) {
            status = vidPnInterface->pfnAssignSourceModeSet(vidPn, sourceId, newSourceModeSet);
            // Assign takes ownership on success and on failures after input validation.
            // Only the three statuses below leave ownership with us.
            if (status != STATUS_GRAPHICS_INVALID_VIDPN && status != STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE &&
                status != STATUS_GRAPHICS_INVALID_VIDPN_SOURCEMODESET) {
                newSourceModeSet = nullptr;
            }
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

        if (newTargetModeSet) {
            status = vidPnInterface->pfnAssignTargetModeSet(vidPn, targetId, newTargetModeSet);
            if (status != STATUS_GRAPHICS_INVALID_VIDPN && status != STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET &&
                status != STATUS_GRAPHICS_INVALID_VIDPN_TARGETMODESET) {
                newTargetModeSet = nullptr;
            }
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

        // DDI publication requirement: use UpdatePathSupportInfo to report the recomputed scaling/rotation
        // capabilities for this path.
        if (supportFieldsModified) {
            status = vidPnTopologyInterface->pfnUpdatePathSupportInfo(vidPnTopology, &localPath);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
    }

    return STATUS_SUCCESS;
}
