#include "device.hpp"

DECLSPEC_PAGED_CODE _Check_return_ _Function_class_DXGK_(DXGKDDI_ADD_DEVICE) _IRQL_requires_DXGK_(
    PASSIVE_LEVEL) NTSTATUS FbDevice::AddDevice(IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject) noexcept {
    PAGED_CODE();
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);
    return STATUS_NOT_IMPLEMENTED;
}
