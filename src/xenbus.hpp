#pragma once

#include "common.hpp"
#include "xen_interface.hpp"

extern "C" {
#include "debug_interface.h"
#include "evtchn_interface.h"
#include "store_interface.h"
#include "suspend_interface.h"
}

template <>
struct XenInterfaceAdapter<XENBUS_DEBUG_INTERFACE> {
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

template <>
struct XenInterfaceAdapter<XENBUS_EVTCHN_INTERFACE> {
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

template <>
struct XenInterfaceAdapter<XENBUS_STORE_INTERFACE> {
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

template <>
struct XenInterfaceAdapter<XENBUS_SUSPEND_INTERFACE> {
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
