#pragma once

#include "common.hpp"
#include "xenbus.hpp"

class VfbFrontend {
public:
    VfbFrontend(StoreHandle store) noexcept : _store(wistd::move(store)) {}

private:
    StoreHandle _store;
};
