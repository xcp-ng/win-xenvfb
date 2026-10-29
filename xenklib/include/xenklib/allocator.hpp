#pragma once

#include <xenklib/common.hpp>
#include <xenklib/type_traits.hpp>

template <typename T, ULONG tag>
class TaggedPoolDeleter {
public:
    _IRQL_requires_max_(DISPATCH_LEVEL) void operator()(_In_opt_ wistd::remove_extent_t<T> *value) {
        if (value) {
            if constexpr (!is_unbounded_array_v<T>) {
                value->~T();
            }
            ExFreePoolWithTag(value, tag);
        }
    }
};

template <typename T, ULONG tag>
using UniqueTaggedPool = wistd::unique_ptr<T, TaggedPoolDeleter<T, tag>>;

template <typename T, ULONG tag, POOL_TYPE poolType = NonPagedPoolNx, bool zeroed = true, typename... Args>
[[nodiscard]] static inline wistd::enable_if_t<!wistd::is_array_v<T>, UniqueTaggedPool<T, tag>>
MakeUniqueTaggedPool(Args &&...args) {
    static_assert(tag != 0, "Pool tag cannot be 0");
    static_assert(alignof(T) <= MEMORY_ALLOCATION_ALIGNMENT, "Type alignment is too large");
    static_assert(wistd::is_nothrow_constructible_v<T, Args &&...>, "Constructor must be nothrow");
    static_assert(wistd::is_nothrow_destructible_v<T>, "Destructor must be nothrow");

    void *storage;
    if constexpr (zeroed) {
        storage = ExAllocatePoolZero(poolType, sizeof(T), tag);
    } else {
        storage = ExAllocatePoolUninitialized(poolType, sizeof(T), tag);
    }
    if (!storage) {
        return {};
    }

    T *object = ::new (storage) T(wistd::forward<Args>(args)...);
    return UniqueTaggedPool<T, tag>{object};
}

template <typename T, ULONG tag, POOL_TYPE poolType = NonPagedPoolNx, bool zeroed = true, typename... Args>
[[nodiscard]] static inline wistd::enable_if_t<is_unbounded_array_v<T>, UniqueTaggedPool<T, tag>>
MakeUniqueTaggedPool(size_t count) {
    using E = wistd::remove_extent_t<T>;

    static_assert(tag != 0, "Pool tag cannot be 0");
    static_assert(alignof(E) <= MEMORY_ALLOCATION_ALIGNMENT, "Type alignment is too large");
    static_assert(wistd::is_nothrow_constructible_v<E>, "Constructor must be nothrow");
    // TaggedPoolDeleter<T[]> does not call destructors since it doesn't store a count
    static_assert(wistd::is_trivially_destructible_v<E>, "Destructor must be trivial");

    if (count == 0) {
        count = 1;
    }

    size_t size;
    auto status = RtlSizeTMult(count, sizeof(E), &size);
    if (!NT_SUCCESS(status)) {
        return {};
    }

    void *storage;
    if constexpr (zeroed) {
        storage = ExAllocatePoolZero(poolType, size, tag);
    } else {
        storage = ExAllocatePoolUninitialized(poolType, size, tag);
    }
    if (!storage) {
        return {};
    }

    E *object = ::new (storage) E[count]();
    return UniqueTaggedPool<T, tag>{object};
}

void __cdecl operator delete(void *p);
void __cdecl operator delete(void *p, size_t s);
void __cdecl operator delete[](void *p);
