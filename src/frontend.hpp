#pragma once

#include "common.hpp"
#include "xenbus.hpp"

class VfbFrontend {
public:
    VfbFrontend(StoreHandle store, ULONG childUid) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        QueryDeviceDescriptor(INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) noexcept;

private:
    StoreHandle _store;
    BYTE _edid[128];
};
