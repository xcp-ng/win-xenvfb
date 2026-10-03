#include "device.hpp"
#include "xenbus.hpp"
#include "vfb.hpp"
#include "frontends.hpp"

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_START_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbDevice::StartDevice(
        IN_PDXGK_START_INFO DxgkStartInfo,
        IN_PDXGKRNL_INTERFACE DxgkInterface,
        OUT_PULONG NumberOfVideoPresentSources,
        OUT_PULONG NumberOfChildren) noexcept {
    NTSTATUS Status;
    PAGED_CODE();

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

        _fm = MakeUniqueVfb<FrontendManager>(wistd::move(storeHandle));
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

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_STOP_DEVICE)
    _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS VfbDevice::StopDevice() noexcept {
    PAGED_CODE();

    _started = false;
    _fm.release();
    StopXen();
    StopDxgk();

    return STATUS_SUCCESS;
}
