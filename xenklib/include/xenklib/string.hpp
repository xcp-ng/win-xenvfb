#pragma once

#include <xenklib/common.hpp>
#include <xenklib/allocator.hpp>

template <typename T>
struct CharTraits {
    static size_t Length(_In_z_ const T *p) {
        size_t result = 0;
        for (; *p; p++, result++)
            ;
        return result;
    }
};

template <>
struct CharTraits<char> {
    static size_t Length(_In_z_ const char *p) {
        return strlen(p);
    }
};

template <>
struct CharTraits<wchar_t> {
    static size_t Length(_In_z_ const wchar_t *p) {
        return wcslen(p);
    }
};

template <typename T, ULONG tag, POOL_TYPE poolType = NonPagedPoolNx>
class TaggedBasicString {
public:
    explicit TaggedBasicString() {}
    explicit TaggedBasicString(_In_reads_opt_(count) const T *p, _In_ size_t count) {
        if (!p) {
            return;
        }
        if (count == SIZE_MAX) {
            return;
        }
        _buffer = MakeUniqueTaggedPool<T[], tag, poolType, false>(count + 1);
        // Multiplication already checked above
        memcpy(_buffer.get(), p, count * sizeof(T));
        _buffer.get()[count] = 0;
    }
    explicit TaggedBasicString(_In_opt_z_ const T *p) : TaggedBasicString(p, p ? CharTraits<T>::Length(p) : 0) {}
    TaggedBasicString(const TaggedBasicString &other) : TaggedBasicString(other._buffer.get()) {}
    TaggedBasicString &operator=(const TaggedBasicString &other) {
        if (this != &other) {
            TaggedBasicString replace(other);
            _buffer = wistd::move(replace._buffer);
        }
        return *this;
    }
    TaggedBasicString(TaggedBasicString &&) = default;
    TaggedBasicString &operator=(TaggedBasicString &&) = default;

    _Null_terminated_ T *get() const {
        return _buffer.get();
    }

private:
    UniqueTaggedPool<T[], tag> _buffer;
};

using VfbString = TaggedBasicString<char, XenvfbTag>;
using VfbWideString = TaggedBasicString<wchar_t, XenvfbTag>;
