#pragma once

#include "common.hpp"
#include "function_traits.hpp"

_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS VfbQueryInterface(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ const GUID *Guid,
    _In_ ULONG Version,
    _Out_ PINTERFACE Interface,
    _In_ ULONG Size) noexcept;

template <typename T>
struct XenTraits;

template <typename T>
concept XenInterfaceType = requires(T t) {
    requires wistd::is_same_v<decltype(t.Interface), INTERFACE>;
    requires wistd::is_same_v<decltype(XenTraits<T>::Acquire(t)), NTSTATUS>;
    XenTraits<T>::Release(t);
    requires wistd::is_convertible_v<decltype(XenTraits<T>::GetGuid()), const GUID &>;
    requires wistd::is_convertible_v<decltype(XenTraits<T>::GetVersion()), ULONG>;
};

template <XenInterfaceType I>
class XenHandle;

// Oddly enough, bus interfaces of xenbus don't actually provide an INTERFACE refcount.
template <XenInterfaceType I>
class XenInterface {
public:
    explicit XenInterface() noexcept {}
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

    static _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        Query(_Out_ XenInterface &Interface, _In_ PDEVICE_OBJECT PhysicalDeviceObject) {
        I interface{};

        auto Status = VfbQueryInterface(
            PhysicalDeviceObject,
            &XenTraits<I>::GetGuid(),
            XenTraits<I>::GetVersion(),
            &interface.Interface,
            sizeof(I));
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        Interface.Reset();
        Interface._data = interface;
        Interface._valid = true;
        return Status;
    }

    template <typename H>
        requires requires(I &data) {
            { H(data, true) } noexcept;
        }
    NTSTATUS Acquire(H &Result) {
        if (!_valid) {
            return STATUS_UNSUCCESSFUL;
        }

        auto Status = XenTraits<I>::Acquire(_data);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        Result = H(_data, true);
        return Status;
    }

    friend void swap(XenInterface &self, XenInterface &other) noexcept {
        self._data = wistd::exchange(other._data, self._data);
        self._valid = wistd::exchange(other._valid, self._valid);
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
    explicit XenHandle() noexcept {}
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

    template <typename F, typename... Args>
    result_type_t<F> operator()(F method, Args &&...args) {
        return method(&_data.Interface, wistd::forward<Args>(args)...);
    }

    friend void swap(XenHandle &self, XenHandle &other) noexcept {
        self._data = wistd::exchange(other._data, self._data);
        self._valid = wistd::exchange(other._valid, self._valid);
    }

protected:
    friend class XenInterface<I>;
    XenHandle(I data, bool valid) noexcept : _data(data), _valid(valid) {}

    I _data{};
    bool _valid = false;

    void dispose() noexcept {
        if (wistd::exchange(_valid, false)) {
            XenTraits<I>::Release(_data);
            _data = {};
        }
    }
};
