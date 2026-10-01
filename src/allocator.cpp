#include "allocator.hpp"
#include "assert.hpp"

// The following deleters should never be called. Allocations should only ever go through our tagged smart pointers.
// MSVC insists on having these anyway.
void __cdecl operator delete(void *p) {
    UNREFERENCED_PARAMETER(p);
    BUG("Unexpected call to unused deleter");
}
void __cdecl operator delete(void *p, size_t size) {
    UNREFERENCED_PARAMETER(p);
    UNREFERENCED_PARAMETER(size);
    BUG("Unexpected call to unused deleter");
}
void __cdecl operator delete[](void *p) {
    UNREFERENCED_PARAMETER(p);
    BUG("Unexpected call to unused deleter");
}
