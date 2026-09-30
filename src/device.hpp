#pragma once

extern "C" {
#include <ntddk.h>

#include <dispmprt.h>
}

class VfbDevice {
public:
    VfbDevice() noexcept {}

    DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_ADD_DEVICE)
        _IRQL_requires_DXGK_(PASSIVE_LEVEL) NTSTATUS AddDevice(IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject) noexcept;
};
