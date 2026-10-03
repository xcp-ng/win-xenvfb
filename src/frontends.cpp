#include "common.hpp"
#include "xenbus.hpp"
#include "dbg_print.hpp"
#include "frontends.hpp"

NTSTATUS FrontendManager::Initialize() noexcept {
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

NTSTATUS FrontendManager::Enumerate() noexcept {
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

void FrontendManager::ThreadWork() noexcept {
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

FrontendManager::~FrontendManager() {
    ThreadAlert();
    ThreadJoin();
    if (_watch) {
        _store(_store->StoreWatchRemove, _watch);
    }
}
