#pragma once

#include <xenklib/common.hpp>
#include <xenklib/xen_interface.hpp>

extern "C" {
#include "debug_interface.h"
#include "evtchn_interface.h"
#include "store_interface.h"
#include "suspend_interface.h"
}

namespace xenklib {

template <>
struct XenTraits<XENBUS_DEBUG_INTERFACE> {
    static NTSTATUS Acquire(XENBUS_DEBUG_INTERFACE i) {
        return i.DebugAcquire(&i.Interface);
    }
    static void Release(XENBUS_DEBUG_INTERFACE i) {
        i.DebugRelease(&i.Interface);
    }
    static const GUID &GetGuid() {
        return GUID_XENBUS_DEBUG_INTERFACE;
    }
    static ULONG GetVersion() {
        return XENBUS_DEBUG_INTERFACE_VERSION_MAX;
    }
};
using DebugInterface = XenInterface<XENBUS_DEBUG_INTERFACE>;
using DebugHandle = XenHandle<XENBUS_DEBUG_INTERFACE>;

template <>
struct XenTraits<XENBUS_EVTCHN_INTERFACE> {
    static NTSTATUS Acquire(XENBUS_EVTCHN_INTERFACE i) {
        return i.EvtchnAcquire(&i.Interface);
    }
    static void Release(XENBUS_EVTCHN_INTERFACE i) {
        i.EvtchnRelease(&i.Interface);
    }
    static const GUID &GetGuid() {
        return GUID_XENBUS_EVTCHN_INTERFACE;
    }
    static ULONG GetVersion() {
        return XENBUS_EVTCHN_INTERFACE_VERSION_MAX;
    }
};
using EvtchnInterface = XenInterface<XENBUS_EVTCHN_INTERFACE>;
using EvtchnHandle = XenHandle<XENBUS_EVTCHN_INTERFACE>;

template <>
struct XenTraits<XENBUS_STORE_INTERFACE> {
    static NTSTATUS Acquire(XENBUS_STORE_INTERFACE i) {
        return i.StoreAcquire(&i.Interface);
    }
    static void Release(XENBUS_STORE_INTERFACE i) {
        i.StoreRelease(&i.Interface);
    }
    static const GUID &GetGuid() {
        return GUID_XENBUS_STORE_INTERFACE;
    }
    static ULONG GetVersion() {
        return XENBUS_STORE_INTERFACE_VERSION_MAX;
    }
};
using StoreInterface = XenInterface<XENBUS_STORE_INTERFACE>;

// note: StoreHandle must outlive any StoreDeleter attached to it (and any dependent resources)
class StoreDeleter {
public:
    explicit StoreDeleter(XenHandle<XENBUS_STORE_INTERFACE> &store) : _store(&store) {}
    void operator()(_In_opt_ PSTR value) {
        (*_store)((*_store)->StoreFree, value);
    }

private:
    XenHandle<XENBUS_STORE_INTERFACE> *_store;
};
using StoreString = wistd::unique_ptr<CHAR, StoreDeleter>;

class StoreHandle : public XenHandle<XENBUS_STORE_INTERFACE> {
public:
    explicit StoreHandle() {}

    StoreString EmptyString() {
        return StoreString(nullptr, StoreDeleter(*this));
    }

    XENBUS_STORE_INTERFACE &operator*() {
        return _data;
    }
    XENBUS_STORE_INTERFACE *operator->() {
        return &_data;
    }

    template <typename F, typename... Args>
    result_type_t<F> operator()(F method, Args &&...args) {
        return method(&_data.Interface, wistd::forward<Args>(args)...);
    }

    NTSTATUS DirectoryTransacted(
        _Out_ StoreString &Result,
        _In_opt_ PXENBUS_STORE_TRANSACTION Transaction,
        _In_opt_ PCSTR Prefix,
        _In_ PCSTR Node);

    NTSTATUS Directory(_Out_ StoreString &Result, _In_opt_ PCSTR Prefix, _In_ PCSTR Node) {
        return DirectoryTransacted(Result, nullptr, Prefix, Node);
    }

private:
    friend StoreInterface;
    StoreHandle(XENBUS_STORE_INTERFACE data, bool valid) : XenHandle<XENBUS_STORE_INTERFACE>(data, valid) {}
};

template <>
struct XenTraits<XENBUS_SUSPEND_INTERFACE> {
    static NTSTATUS Acquire(XENBUS_SUSPEND_INTERFACE i) {
        return i.Acquire(&i.Interface);
    }
    static void Release(XENBUS_SUSPEND_INTERFACE i) {
        i.Release(&i.Interface);
    }
    static const GUID &GetGuid() {
        return GUID_XENBUS_SUSPEND_INTERFACE;
    }
    static ULONG GetVersion() {
        return XENBUS_SUSPEND_INTERFACE_VERSION_MAX;
    }
};
using SuspendInterface = XenInterface<XENBUS_SUSPEND_INTERFACE>;
using SuspendHandle = XenHandle<XENBUS_SUSPEND_INTERFACE>;

} // namespace xenklib
