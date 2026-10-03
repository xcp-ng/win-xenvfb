#pragma once

#include "common.hpp"
#include "xenbus.hpp"
#include "allocator.hpp"
#include "frontends.hpp"

class VfbDevice {
public:
    VfbDevice(IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject) noexcept : _pdo(PhysicalDeviceObject) {}
    VfbDevice(const VfbDevice &) = delete;
    VfbDevice &operator=(const VfbDevice &) = delete;
    VfbDevice(VfbDevice &&) = delete;
    VfbDevice &operator=(VfbDevice &&) = delete;
    ~VfbDevice() {}

    DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_START_DEVICE)
        _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS StartDevice(
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

    DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_STOP_DEVICE)
        _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS StopDevice() noexcept;

private:
    const PDEVICE_OBJECT _pdo = nullptr;

    DXGKRNL_INTERFACE _dxgk{};

    DebugInterface _debug{};
    EvtchnInterface _evtchn{};
    StoreInterface _store{};
    SuspendInterface _suspend{};

    UniqueVfb<FrontendManager> _fm;

    bool _started = false;
};
