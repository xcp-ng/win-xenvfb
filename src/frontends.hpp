#pragma once

#include "common.hpp"
#include "xenbus.hpp"
#include "kthread.hpp"
#include "frontend.hpp"
#include "allocator.hpp"
#include "mrsw.hpp"

class VfbFrontendManager : protected KThread {
public:
    static constexpr size_t MaxViews = 1;
    static constexpr size_t MaxChildren = 1;

    explicit VfbFrontendManager(StoreHandle &&store) : _store(wistd::move(store)) {}
    VfbFrontendManager(const VfbFrontendManager &) = delete;
    VfbFrontendManager &operator=(const VfbFrontendManager &) = delete;
    VfbFrontendManager(VfbFrontendManager &&) = delete;
    VfbFrontendManager &operator=(VfbFrontendManager &&) = delete;
    ~VfbFrontendManager();

    NTSTATUS Initialize();

    _IRQL_requires_max_(APC_LEVEL) bool HasChild(ULONG uid) {
        auto lock = _lock.AcquireShared();
        if (uid >= MaxChildren) {
            return false;
        }
        return !!_frontends[uid];
    }

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        QueryDeviceDescriptor(IN_ULONG ChildUid, INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        SetPowerState(IN_ULONG DeviceUid, IN_DEVICE_POWER_STATE DevicePowerState, IN_POWER_ACTION ActionType);

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS IsSupportedVidPn(
        INOUT_PDXGKARG_ISSUPPORTEDVIDPN pIsSupportedVidPn,
        _In_ const DXGK_VIDPN_INTERFACE *vidPnInterface,
        _In_ D3DKMDT_HVIDPNTOPOLOGY vidPnTopology,
        _In_ const DXGK_VIDPNTOPOLOGY_INTERFACE *vidPnTopologyInterface);

    DECLSPEC_PAGED_CODE
    _Check_return_ _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS EnumVidPnCofuncModality(
        IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality,
        _In_ const DXGK_VIDPN_INTERFACE *vidPnInterface,
        _In_ D3DKMDT_HVIDPNTOPOLOGY vidPnTopology,
        _In_ const DXGK_VIDPNTOPOLOGY_INTERFACE *vidPnTopologyInterface);

protected:
    _IRQL_requires_(PASSIVE_LEVEL) virtual void ThreadWork() override;

private:
    NTSTATUS Enumerate();

    StoreHandle _store;
    PXENBUS_STORE_WATCH _watch = nullptr;

    // Globally locks the frontend list and frontend creation/deletion.
    // Does not lock the VfbFrontend itself!
    MrswLock _lock;
    UniqueVfb<VfbFrontend> _frontends[MaxChildren];
};
