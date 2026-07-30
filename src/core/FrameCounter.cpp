#include "core/PrecompiledHeader.hpp"
#include "FrameCounter.hpp"

namespace Global {
    std::atomic<uint64_t> frameCounter = 0;
}
