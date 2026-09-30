#pragma once

#include "common.hpp"
#include <algorithm>

_IRQL_requires_(PASSIVE_LEVEL) NTSTATUS VfbQueryInterface(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ const GUID *Guid,
    _In_ ULONG Version,
    _Out_ PINTERFACE Interface,
    _In_ ULONG Size) noexcept;

template <typename T>
concept InterfaceType = requires(T t) { std::same_as<decltype(t.Interface), INTERFACE>; };

template <InterfaceType I>
class Interface {
public:
    Interface() noexcept {}
    Interface(const Interface &) = delete;
    Interface &operator=(const Interface &) = delete;
    Interface(Interface &&other) noexcept {
        swap(*this, other);
    }
    Interface &operator=(Interface &&other) noexcept {
        if (this != wistd::addressof(other)) {
            dispose();
            swap(*this, other);
        }
        return *this;
    }
    ~Interface() noexcept {
        dispose();
    }

    static _IRQL_requires_(PASSIVE_LEVEL) NTSTATUS
        Query(Interface &Interface, PDEVICE_OBJECT PhysicalDeviceObject, const GUID &Guid, ULONG Version) {
        INTERFACE interface{};

        auto Status = VfbQueryInterface(PhysicalDeviceObject, &Guid, Version, &interface, sizeof(I));
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        Interface._data = interface;
        Interface._valid = true;
        return Status;
    }

    operator bool() const {
        return _valid;
    }
    void Reset() noexcept {
        dispose();
    }

    I &operator*() const noexcept {
        return _data;
    }
    I *operator->() const noexcept {
        return &_data;
    }

    friend void swap(Interface &self, Interface &other) noexcept {
        using std::swap;
        swap(self._data, other._data);
        swap(self._valid, other._valid);
    }

private:
    I _data{};
    bool _valid = false;

    void dispose() noexcept {
        if (std::exchange(_valid, false)) {
            _data = {};
        }
    }
};
