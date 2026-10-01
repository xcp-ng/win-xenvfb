#include "common.hpp"
#include "assert.hpp"

_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS VfbQueryInterface(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ const GUID *Guid,
    _In_ ULONG Version,
    _Out_ PINTERFACE Interface,
    _In_ ULONG Size) noexcept {
    KEVENT Event;
    IO_STATUS_BLOCK StatusBlock;
    PIRP Irp;
    PIO_STACK_LOCATION StackLocation;
    NTSTATUS Status;

    ASSERT3U(KeGetCurrentIrql(), ==, PASSIVE_LEVEL);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    RtlZeroMemory(&StatusBlock, sizeof(IO_STATUS_BLOCK));

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, PhysicalDeviceObject, NULL, 0, NULL, &Event, &StatusBlock);

    Status = STATUS_UNSUCCESSFUL;
    if (Irp == NULL)
        goto fail1;

    // suppress "uninitialized *Interface" warning when IoCallDriver succeeds
    RtlZeroMemory(Interface, sizeof(*Interface));

    StackLocation = IoGetNextIrpStackLocation(Irp);
    StackLocation->MinorFunction = IRP_MN_QUERY_INTERFACE;

    StackLocation->Parameters.QueryInterface.InterfaceType = Guid;
    StackLocation->Parameters.QueryInterface.Size = (USHORT)Size;
    StackLocation->Parameters.QueryInterface.Version = (USHORT)Version;
    StackLocation->Parameters.QueryInterface.Interface = Interface;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Status = IoCallDriver(PhysicalDeviceObject, Irp);
    if (Status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = StatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        goto fail2;
    }

    return STATUS_SUCCESS;

fail2:
    Error("fail2\n");

fail1:
    Error("fail1 (%08x)\n", Status);

    return Status;
}
