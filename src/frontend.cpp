#include "common.hpp"
#include "frontend.hpp"
#include "edid.hpp"

VfbFrontend::VfbFrontend(StoreHandle store, ULONG childUid) noexcept : _store(wistd::move(store)) {
    VfbGetEdid(childUid, _edid);
}

bool VfbFrontend::IsSizeValid(D3DKMDT_2DREGION size) const noexcept {
    if (size.cx < 640 || size.cy < 480) {
        return false;
    }
    // TODO
    return true;
}


DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontend::QueryDeviceDescriptor(INOUT_PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor) noexcept {
    if (DeviceDescriptor->DescriptorOffset >= sizeof(_edid)) {
        return STATUS_MONITOR_NO_MORE_DESCRIPTOR_DATA;
    }

    RtlCopyMemory(
        DeviceDescriptor->DescriptorBuffer,
        _edid + DeviceDescriptor->DescriptorOffset,
        wistd::min(
            DeviceDescriptor->DescriptorLength,
            static_cast<ULONG>(sizeof(_edid)) - DeviceDescriptor->DescriptorOffset));
    return STATUS_SUCCESS;
}

DECLSPEC_PAGED_CODE _Check_return_ _IRQL_requires_DXGK_(PASSIVE_LEVEL)
NTSTATUS
VfbFrontend::SetPowerState(IN_DEVICE_POWER_STATE DevicePowerState, IN_POWER_ACTION ActionType) noexcept {
    UNREFERENCED_PARAMETER(DevicePowerState);
    UNREFERENCED_PARAMETER(ActionType);

    return STATUS_NOT_IMPLEMENTED;
}
