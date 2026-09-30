#pragma once

extern "C" {
#include <ntddk.h>
}

#include <wil/resource.h>

template <typename T, ULONG tag>
struct TaggedPoolDeleter {
    _IRQL_requires_max_(DISPATCH_LEVEL) void operator()(_In_opt_ T *value) noexcept {
        if (value) {
            value->~T();
            ExFreePoolWithTag(value, tag);
        }
    }
};

template <typename T, ULONG tag>
using UniqueTaggedPool = wistd::unique_ptr<T, TaggedPoolDeleter<T, tag>>;

template <typename T, ULONG tag, POOL_TYPE poolType = NonPagedPoolNx, typename... Args>
WI_NODISCARD static inline UniqueTaggedPool<T, tag> MakeUniqueTaggedPoolZero(Args &&...args) noexcept {
    static_assert(tag != 0, "Pool tag cannot be 0");
    static_assert(alignof(T) <= MEMORY_ALLOCATION_ALIGNMENT, "Type alignment is too large");
    static_assert(wistd::is_nothrow_constructible_v<T, Args &&...>, "Constructor must be nothrow");
    static_assert(wistd::is_nothrow_destructible_v<T>, "Destructor must be nothrow");

    void *storage = ExAllocatePoolZero(poolType, sizeof(T), tag);
    if (!storage) {
        return {};
    }

    T *object = ::new (storage) T(wistd::forward<Args>(args)...);
    return UniqueTaggedPool<T, tag>{object};
}

static constexpr ULONG XenfbTag = 'bfnX';

template <typename T>
using UniqueXenfb = UniqueTaggedPool<T, XenfbTag>;

template <typename T, typename... Args>
WI_NODISCARD static inline UniqueXenfb<T> MakeUniqueXenfb(Args &&...args) {
    return MakeUniqueTaggedPoolZero<T, XenfbTag>(wistd::forward<Args>(args)...);
}
