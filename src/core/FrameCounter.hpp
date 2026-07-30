#pragma once
#include "core/PrecompiledHeader.hpp"

#include <atomic>

namespace Global {
    extern std::atomic<uint64_t> frameCounter;
}
