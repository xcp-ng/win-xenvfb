#include "common.hpp"
#include "xenbus.hpp"
#include "dbg_print.hpp"
#include "frontends.hpp"

NTSTATUS VfbFrontendManager::Initialize() noexcept {
    NTSTATUS Status;

    Status = _store(_store->StoreWatchAdd, const_cast<PSTR>("device"), const_cast<PSTR>("vfb"), &_threadEvent, &_watch);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    auto cleanupWatch = wil::scope_exit([&]() { _store(_store->StoreWatchRemove, _watch); });

    Status = ThreadStart();
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    cleanupWatch.release();
    return STATUS_SUCCESS;
}

NTSTATUS VfbFrontendManager::Enumerate() noexcept {
    NTSTATUS Status;

    StoreString paths = _store.EmptyString();
    Status = _store.Directory(paths, "device", "vfb");
    // STATUS_OBJECT_PATH_NOT_FOUND == "this is a leaf"
    // STATUS_OBJECT_NAME_NOT_FOUND == "this does not exist"
    if (Status == STATUS_OBJECT_PATH_NOT_FOUND || Status == STATUS_OBJECT_NAME_NOT_FOUND) {}
    if (!NT_SUCCESS(Status)) {
        return Status;
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
    auto lock = _lock.acquire();

    if (ChildUid >= ARRAYSIZE(_frontends)) {
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
    UNREFERENCED_PARAMETER(DeviceUid);
    UNREFERENCED_PARAMETER(DevicePowerState);
    UNREFERENCED_PARAMETER(ActionType);

    return STATUS_NOT_IMPLEMENTED;
}
