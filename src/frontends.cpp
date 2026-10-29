#include "common.hpp"
#include "xenbus.hpp"
#include "dbg_print.hpp"
#include "frontends.hpp"

NTSTATUS VfbFrontendManager::Initialize() {
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

NTSTATUS VfbFrontendManager::Enumerate() {
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

_IRQL_requires_(PASSIVE_LEVEL) void VfbFrontendManager::ThreadWork() {
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
VfbFrontendManager::QueryDeviceDescriptor(IN_ULONG ChildUid, INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) {
    PAGED_CODE();

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
    IN_POWER_ACTION ActionType) {
    PAGED_CODE();

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
    _In_ const DXGK_VIDPNTOPOLOGY_INTERFACE *vidPnTopologyInterface) {
    PAGED_CODE();

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

    auto lock = _lock.AcquireShared();
    bool encountered[MaxViews][MaxChildren] = {};
    const auto vidPn = pIsSupportedVidPn->hDesiredVidPn;

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
        // Informational end statuses do not provide a descriptor.
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

        const auto sourceId = path->VidPnSourceId;
        const auto targetId = path->VidPnTargetId;
        if (sourceId >= MaxViews || targetId >= MaxChildren) {
            return STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY;
        }
        if (sourceId != targetId || wistd::exchange(encountered[sourceId][targetId], true)) {
            return STATUS_SUCCESS;
        }
        if (!VfbFrontend::IsPathSupported(*path)) {
            return STATUS_SUCCESS;
        }

        D3DKMDT_HVIDPNSOURCEMODESET sourceModeSet = nullptr;
        const DXGK_VIDPNSOURCEMODESET_INTERFACE *sourceModeSetInterface = nullptr;
        status = vidPnInterface->pfnAcquireSourceModeSet(vidPn, sourceId, &sourceModeSet, &sourceModeSetInterface);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        auto cleanupSourceModeSet =
            wil::scope_exit([&]() { vidPnInterface->pfnReleaseSourceModeSet(vidPn, sourceModeSet); });

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

        D3DKMDT_HVIDPNTARGETMODESET targetModeSet = nullptr;
        const DXGK_VIDPNTARGETMODESET_INTERFACE *targetModeSetInterface = nullptr;
        status = vidPnInterface->pfnAcquireTargetModeSet(vidPn, targetId, &targetModeSet, &targetModeSetInterface);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        auto cleanupTargetModeSet =
            wil::scope_exit([&]() { vidPnInterface->pfnReleaseTargetModeSet(vidPn, targetModeSet); });

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

        if (!VfbFrontend::AreModesCompatible(pinnedSourceMode, pinnedTargetMode)) {
            return STATUS_SUCCESS;
        }
    }

    pIsSupportedVidPn->IsVidPnSupported = TRUE;
    return STATUS_SUCCESS;
}

// Preserve pinned and pivot mode sets. Rebuild the other sets with all supported modes that can work with the preserved
// modes and path settings.
//
// The DDI requires us to:
// - Inspect the topology and existing modes to find those constraints.
// - Preserve pinned mode sets and any mode set designated as the pivot.
// - Make each unpinned, non-pivot mode set the largest possible set compatible with the constraints.
// - Update unpinned scaling/rotation support, except for a transformation pivot.
//
// This function implements the inspection, preservation and publication steps. VfbFrontend implements the path and
// mode constraints and compatible-mode enumeration.
DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontendManager::EnumVidPnCofuncModality(
    IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality,
    _In_ const DXGK_VIDPN_INTERFACE *vidPnInterface,
    _In_ D3DKMDT_HVIDPNTOPOLOGY vidPnTopology,
    _In_ const DXGK_VIDPNTOPOLOGY_INTERFACE *vidPnTopologyInterface) {
    PAGED_CODE();

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
        // Frontend policy operates on a copy because the acquired descriptor is read-only.
        auto localPath = *path;
        bool supportFieldsModified = false;
        status = VfbFrontend::PrepareCofuncPath(localPath, *pEnumCofuncModality, &supportFieldsModified);
        if (!NT_SUCCESS(status)) {
            return status;
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

            if (!VfbFrontend::AreModesCompatible(pinnedSourceMode, pinnedTargetMode)) {
                return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
            }

            // Besides resolution, the DDI constraints include path transformations, target color basis/ranges, etc.
            // Pass them all to AddSourceModes/AddTargetModes.
            // constraints is valid in this scope only.
            const VidPnCofuncConstraints constraints = {
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
                status = VfbFrontend::AddSourceModes(newSourceModeSet, newSourceModeSetInterface, constraints);
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
                status = VfbFrontend::AddTargetModes(newTargetModeSet, newTargetModeSetInterface, constraints);
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
