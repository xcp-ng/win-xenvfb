#include "common.hpp"
#include "frontend.hpp"
#include "edid.hpp"

VfbFrontend::VfbFrontend(StoreHandle store, ULONG childUid) noexcept : _store(wistd::move(store)) {
    VfbGetEdid(childUid, _edid);
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
