#pragma once

// IWYU pragma: always_keep

// IWYU pragma: begin_exports

extern "C" {
#include <ntifs.h>
#include <ntintsafe.h>
}

// attention: most STL headers cannot be used in kernel mode
#include <stdarg.h>

#include <wil/resource.h>
#include <wil/result_macros.h>

// IWYU pragma: end_exports
