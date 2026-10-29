#include <xenklib/common.hpp>
#include <xenklib/xenbus.hpp>

NTSTATUS StoreHandle::DirectoryTransacted(
    _Out_ StoreString &Result,
    _In_opt_ PXENBUS_STORE_TRANSACTION Transaction,
    _In_opt_ PCSTR Prefix,
    _In_ PCSTR Node) {
    PSTR pResult = nullptr;
    NTSTATUS status;

    status = (*this)((*this)->StoreDirectory, Transaction, const_cast<PSTR>(Prefix), const_cast<PSTR>(Node), &pResult);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    Result = StoreString(pResult, StoreDeleter(*this));
    return status;
}
