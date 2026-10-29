#include <xenklib/common.hpp>
#include <xenklib/kthread.hpp>
#include <xenklib/assert.hpp>

namespace xenklib {

EXTERN_C _IRQL_requires_same_ _Function_class_(KSTART_ROUTINE) VOID KThreadStartRoutine(_In_ PVOID StartContext) {
    auto worker = static_cast<KThread *>(StartContext);
    if (!worker) {
        return;
    }
    worker->ThreadWork();
}

KThread::KThread() {
    KeInitializeEvent(&_threadEvent, NotificationEvent, FALSE);
}

_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS KThread::ThreadStart() {
    HANDLE Handle;
    NTSTATUS status;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (_thread) {
        return STATUS_UNSUCCESSFUL;
    }

    status = PsCreateSystemThread(
        &Handle,
        STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
        NULL,
        NULL,
        NULL,
        KThreadStartRoutine,
        this);
    if (!NT_SUCCESS(status)) {
        goto fail_thread;
    }

    status = ObReferenceObjectByHandle(
        Handle,
        SYNCHRONIZE,
        *PsThreadType,
        KernelMode,
        reinterpret_cast<PVOID *>(&_thread),
        nullptr);
    if (!NT_SUCCESS(status))
        goto fail_ref;

    ZwClose(Handle);

    return STATUS_SUCCESS;

fail_ref:
    Error("fail_ref\n");

    ThreadAlert();
    ZwWaitForSingleObject(Handle, FALSE, nullptr);
    ZwClose(Handle);

fail_thread:
    Error("fail_thread (%08x)\n", status);

    return status;
}

_IRQL_requires_(PASSIVE_LEVEL) void KThread::ThreadJoin() {
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT3P(KeGetCurrentThread(), !=, _thread);

    if (_thread) {
        (VOID) KeWaitForSingleObject(_thread, Executive, KernelMode, FALSE, NULL);
        _threadJoined = true;
    }
}

KThread::~KThread() {
    if (_thread) {
        ASSERT(_threadJoined);
        ObDereferenceObject(_thread);
    }
}

} // namespace xenklib
