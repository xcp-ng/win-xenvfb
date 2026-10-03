#pragma once

#include "common.hpp"
#include "xenbus.hpp"
#include "vfb.hpp"
#include "kthread.hpp"
#include "frontend.hpp"
#include "allocator.hpp"

class VfbFrontendManager : protected KThread {
public:
    VfbFrontendManager(StoreHandle &&store) noexcept : _store(wistd::move(store)) {}
    VfbFrontendManager(const VfbFrontendManager &) = delete;
    VfbFrontendManager &operator=(const VfbFrontendManager &) = delete;
    VfbFrontendManager(VfbFrontendManager &&) = delete;
    VfbFrontendManager &operator=(VfbFrontendManager &&) = delete;
    ~VfbFrontendManager();

    NTSTATUS Initialize() noexcept;

    _IRQL_requires_max_(APC_LEVEL) bool HasChild(ULONG uid) {
        auto lock = _lock.acquire();
        if (uid >= ARRAYSIZE(_frontends)) {
            return false;
        }
        return _frontends[uid].get();
    }

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        QueryDeviceDescriptor(IN_ULONG ChildUid, INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        SetPowerState(IN_ULONG DeviceUid, IN_DEVICE_POWER_STATE DevicePowerState, IN_POWER_ACTION ActionType) noexcept;

protected:
    virtual void ThreadWork() noexcept override;

private:
    NTSTATUS Enumerate() noexcept;

    StoreHandle _store;
    PXENBUS_STORE_WATCH _watch = nullptr;

    // Globally locks the frontend list and frontend creation/deletion.
    // Does not lock the VfbFrontend itself!
    wil::fast_mutex _lock;
    UniqueVfb<VfbFrontend> _frontends[VfbMaxChildren];
};
