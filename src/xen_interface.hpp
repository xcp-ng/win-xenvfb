#pragma once

#include "common.hpp"

_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS VfbQueryInterface(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ const GUID *Guid,
    _In_ ULONG Version,
    _Out_ PINTERFACE Interface,
    _In_ ULONG Size) noexcept;

template <typename T>
struct XenInterfaceAdapter;

template <typename T, typename U>
static inline constexpr bool IsSame = false;
template <typename T>
inline constexpr bool IsSame<T, T> = true;

template <typename T>
concept XenInterfaceType = requires(T t) {
    requires IsSame<decltype(t.Interface), INTERFACE>;
    requires IsSame<decltype(XenInterfaceAdapter<T>::Acquire(t)), NTSTATUS>;
    XenInterfaceAdapter<T>::Release(t);
    XenInterfaceAdapter<T>::GetGuid();
    XenInterfaceAdapter<T>::GetVersion();
};

template <XenInterfaceType I>
class XenHandle;

// Oddly enough, bus interfaces of xenbus don't actually provide an INTERFACE refcount.
template <XenInterfaceType I>
class XenInterface {
public:
    XenInterface() noexcept {}
    XenInterface(const XenInterface &other) = delete;
    XenInterface &operator=(const XenInterface &other) = delete;
    XenInterface(XenInterface &&other) noexcept {
        swap(*this, other);
    }
    XenInterface &operator=(XenInterface &&other) noexcept {
        if (this != wistd::addressof(other)) {
            dispose();
            swap(*this, other);
        }
        return *this;
    }
    ~XenInterface() noexcept {
        dispose();
    }

    operator bool() const {
        return _valid;
    }
    void Reset() noexcept {
        dispose();
    }

    static _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS Query(XenInterface &Interface, PDEVICE_OBJECT PhysicalDeviceObject) {
        I interface{};

        auto Status = VfbQueryInterface(
            PhysicalDeviceObject,
            &XenInterfaceAdapter<I>::GetGuid(),
            XenInterfaceAdapter<I>::GetVersion(),
            &interface.Interface,
            sizeof(I));
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        Interface._data = interface;
        Interface._valid = true;
        return Status;
    }

    NTSTATUS Acquire(XenHandle<I> &Result) {
        if (!_valid) {
            return STATUS_UNSUCCESSFUL;
        }

        auto Status = XenInterfaceAdapter<I>::Acquire(_data);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        Result = wistd::move(XenHandle<I>(_data, true));
        return Status;
    }

    friend void swap(XenInterface &self, XenInterface &other) noexcept {
        using wistd::swap;
        swap(self._data, other._data);
        swap(self._valid, other._valid);
    }

private:
    I _data{};
    bool _valid = false;

    void dispose() noexcept {
        if (wistd::exchange(_valid, false)) {
            if (_data.Interface.InterfaceDereference) {
                _data.Interface.InterfaceDereference(_data.Interface.Context);
            }
            _data = {};
        }
    }
};

// acquisition of the resource itself (e.g. StoreAcquire etc)
template <XenInterfaceType I>
class XenHandle {
public:
    XenHandle() noexcept {}
    XenHandle(const XenHandle &other) = delete;
    XenHandle &operator=(const XenHandle &other) = delete;
    XenHandle(XenHandle &&other) noexcept {
        swap(*this, other);
    }
    XenHandle &operator=(XenHandle &&other) noexcept {
        if (this != wistd::addressof(other)) {
            dispose();
            swap(*this, other);
        }
        return *this;
    }
    ~XenHandle() noexcept {
        dispose();
    }

    operator bool() const {
        return _valid;
    }
    void Reset() noexcept {
        dispose();
    }

    I &operator*() noexcept {
        return _data;
    }
    I *operator->() noexcept {
        return &_data;
    }

    friend void swap(XenHandle &self, XenHandle &other) noexcept {
        using wistd::swap;
        swap(self._data, other._data);
        swap(self._valid, other._valid);
    }

private:
    friend class XenInterface<I>;
    XenHandle(I data, bool valid) noexcept : _data(data), _valid(valid) {}

    I _data{};
    bool _valid = false;

    void dispose() noexcept {
        if (wistd::exchange(_valid, false)) {
            XenInterfaceAdapter<I>::Release(_data);
            _data = {};
        }
    }
};
