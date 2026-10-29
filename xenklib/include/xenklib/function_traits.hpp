#pragma once

#include <xenklib/common.hpp>

template <typename F>
struct function_traits {};

template <typename R, typename... T>
struct function_traits<wistd::function<R(T...)>> {
    using result_type = R;
};

template <typename R, typename... T>
struct function_traits<R(T...)> {
    using result_type = R;
};

template <typename R, typename... T>
struct function_traits<R (*)(T...)> {
    using result_type = R;
};

template <typename F>
using result_type_t = typename function_traits<F>::result_type;
