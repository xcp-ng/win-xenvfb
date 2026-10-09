#pragma once

#include "common.hpp"

template <class T>
struct is_unbounded_array : wistd::false_type {};

template <class T>
struct is_unbounded_array<T[]> : wistd::true_type {};

template <class T>
constexpr bool is_unbounded_array_v = is_unbounded_array<T>::value;
