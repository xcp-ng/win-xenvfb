#pragma once

#include <xenklib/common.hpp>
#include <xenklib/function_traits.hpp>

_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS XenQueryInterface(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ const GUID *Guid,
    _In_ ULONG Version,
    _Out_ PINTERFACE Interface,
    _In_ ULONG Size);

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
    explicit XenInterface() {}
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
    ~XenInterface() {
        dispose();
    }

    operator bool() const {
        return _valid;
    }
    void Reset() {
        dispose();
    }

    static _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        Query(_Out_ XenInterface &Interface, _In_ PDEVICE_OBJECT PhysicalDeviceObject) {
        I interface{};

        auto Status = XenQueryInterface(
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
            { H(data, true) };
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

    friend void swap(XenInterface &self, XenInterface &other) {
        self._data = wistd::exchange(other._data, self._data);
        self._valid = wistd::exchange(other._valid, self._valid);
    }

private:
    I _data{};
    bool _valid = false;

    void dispose() {
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
    explicit XenHandle() {}
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
    ~XenHandle() {
        dispose();
    }

    operator bool() const {
        return _valid;
    }
    void Reset() {
        dispose();
    }

    I &operator*() {
        return _data;
    }
    I *operator->() {
        return &_data;
    }

    template <typename F, typename... Args>
    result_type_t<F> operator()(F method, Args &&...args) {
        return method(&_data.Interface, wistd::forward<Args>(args)...);
    }

    friend void swap(XenHandle &self, XenHandle &other) {
        self._data = wistd::exchange(other._data, self._data);
        self._valid = wistd::exchange(other._valid, self._valid);
    }

protected:
    friend class XenInterface<I>;
    XenHandle(I data, bool valid) : _data(data), _valid(valid) {}

    I _data{};
    bool _valid = false;

    void dispose() {
        if (wistd::exchange(_valid, false)) {
            XenTraits<I>::Release(_data);
            _data = {};
        }
    }
};
