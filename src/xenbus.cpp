#include "common.hpp"
#include "xenbus.hpp"

NTSTATUS StoreHandle::DirectoryTransacted(
    _Out_ StoreString &Result,
    _In_opt_ PXENBUS_STORE_TRANSACTION Transaction,
    _In_opt_ PCSTR Prefix,
    _In_ PCSTR Node) noexcept {
    PSTR pResult = nullptr;
    NTSTATUS Status;

    Status = (*this)((*this)->StoreDirectory, Transaction, const_cast<PSTR>(Prefix), const_cast<PSTR>(Node), &pResult);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Result = StoreString(pResult, StoreDeleter(*this));
    return Status;
}
