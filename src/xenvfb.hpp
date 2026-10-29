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
using UniqueVfb = xenklib::UniqueTaggedPool<T, XenvfbTag>;

template <typename T, typename... Args>
[[nodiscard]] static inline UniqueVfb<T> MakeUniqueVfb(Args &&...args) {
    return xenklib::MakeUniqueTaggedPool<T, XenvfbTag, NonPagedPoolNx, true>(wistd::forward<Args>(args)...);
}

using VfbString = xenklib::TaggedBasicString<char, XenvfbTag>;
using VfbWideString = xenklib::TaggedBasicString<wchar_t, XenvfbTag>;
