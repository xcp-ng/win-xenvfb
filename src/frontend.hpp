#pragma once

#include "common.hpp"
#include "xenbus.hpp"

class VfbFrontend {
public:
    static constexpr size_t Bpp = 32;
    static constexpr size_t PixelBytes = Bpp / 8;

    explicit VfbFrontend(StoreHandle store, ULONG childUid) noexcept;

    bool IsSizeValid(D3DKMDT_2DREGION size) const noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        QueryDeviceDescriptor(INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) noexcept;

    DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS
        SetPowerState(IN_DEVICE_POWER_STATE DevicePowerState, IN_POWER_ACTION ActionType) noexcept;

private:
    StoreHandle _store;
    BYTE _edid[128];
};
