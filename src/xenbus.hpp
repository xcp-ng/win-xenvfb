#pragma once

#include "common.hpp"
#include "interface.hpp"

extern "C" {
#include "debug_interface.h"
}

using XenbusDebugInterface = Interface<XENBUS_DEBUG_INTERFACE>;
