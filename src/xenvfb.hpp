#pragma once

// IWYU pragma: always_keep

// IWYU pragma: begin_exports

#include <xenklib/common.hpp>

extern "C" {
#include <dispmprt.h>
}

#include <xenklib/allocator.hpp>
#include <xenklib/string.hpp>

// IWYU pragma: end_exports

static constexpr ULONG XenvfbTag = 'bfvX';

template <typename T>
using UniqueVfb = UniqueTaggedPool<T, XenvfbTag>;

template <typename T, typename... Args>
[[nodiscard]] static inline UniqueVfb<T> MakeUniqueVfb(Args &&...args) {
    return MakeUniqueTaggedPool<T, XenvfbTag, NonPagedPoolNx, true>(wistd::forward<Args>(args)...);
}

using VfbString = TaggedBasicString<char, XenvfbTag>;
using VfbWideString = TaggedBasicString<wchar_t, XenvfbTag>;
