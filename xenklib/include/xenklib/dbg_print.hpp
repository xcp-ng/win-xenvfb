/* Copyright (c) Xen Project.
 * Copyright (c) Cloud Software Group, Inc.
 * Copyright (c) Vates.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 * *   Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the
 *     following disclaimer.
 * *   Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the
 *     following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <xenklib/common.hpp>

namespace xenklib {

#define stringify_literal(_text) #_text
#define stringify(_text) stringify_literal(_text)
#define __MODULE__ stringify(PROJECT)

#pragma warning(disable : 4127) // conditional expression is constant

static __inline VOID __Error(_In_ PCSTR Prefix, _In_ PCSTR Format, ...) {
    va_list Arguments;

    va_start(Arguments, Format);

#pragma prefast(suppress : 6001) // Using uninitialized memory
    vDbgPrintExWithPrefix(Prefix, DPFLTR_IHVVIDEO_ID, DPFLTR_ERROR_LEVEL, Format, Arguments);
    va_end(Arguments);
}

#define Error(...) xenklib::__Error(__MODULE__ "|" __FUNCTION__ ": ", __VA_ARGS__)

static __inline VOID __Warning(_In_ PCSTR Prefix, _In_ PCSTR Format, ...) {
    va_list Arguments;

    va_start(Arguments, Format);

#pragma prefast(suppress : 6001) // Using uninitialized memory
    vDbgPrintExWithPrefix(Prefix, DPFLTR_IHVVIDEO_ID, DPFLTR_WARNING_LEVEL, Format, Arguments);
    va_end(Arguments);
}

#define Warning(...) xenklib::__Warning(__MODULE__ "|" __FUNCTION__ ": ", __VA_ARGS__)

static __inline VOID __Trace(_In_ PCSTR Prefix, _In_ PCSTR Format, ...) {
    va_list Arguments;

    va_start(Arguments, Format);

#pragma prefast(suppress : 6001) // Using uninitialized memory
    vDbgPrintExWithPrefix(Prefix, DPFLTR_IHVVIDEO_ID, DPFLTR_TRACE_LEVEL, Format, Arguments);
    va_end(Arguments);
}

#define Trace(...) xenklib::__Trace(__MODULE__ "|" __FUNCTION__ ": ", __VA_ARGS__)
#if DBG
#define DebugTrace(...) Trace(__VA_ARGS__)
#else
#define DebugTrace(...)
#endif

static __inline VOID __Info(_In_ PCSTR Prefix, _In_ PCSTR Format, ...) {
    va_list Arguments;

    va_start(Arguments, Format);

#pragma prefast(suppress : 6001) // Using uninitialized memory
    vDbgPrintExWithPrefix(Prefix, DPFLTR_IHVVIDEO_ID, DPFLTR_INFO_LEVEL, Format, Arguments);
    va_end(Arguments);
}

#define Info(...) xenklib::__Info(__MODULE__ "|" __FUNCTION__ ": ", __VA_ARGS__)
#if DBG
#define DebugInfo(...) Info(__VA_ARGS__)
#else
#define DebugInfo(...)
#endif

#define DBG_PRINT_ENABLE_FILTER(_Id, _Level) \
    do { \
        DbgSetDebugFilterState((_Id), (_Level), TRUE); \
    } while (FALSE)

static __inline VOID __DbgPrintEnable(VOID) {
    DBG_PRINT_ENABLE_FILTER(DPFLTR_IHVVIDEO_ID, DPFLTR_ERROR_LEVEL);
    DBG_PRINT_ENABLE_FILTER(DPFLTR_IHVVIDEO_ID, DPFLTR_WARNING_LEVEL);
    DBG_PRINT_ENABLE_FILTER(DPFLTR_IHVVIDEO_ID, DPFLTR_TRACE_LEVEL);
    DBG_PRINT_ENABLE_FILTER(DPFLTR_IHVVIDEO_ID, DPFLTR_INFO_LEVEL);
}

} // namespace xenklib
