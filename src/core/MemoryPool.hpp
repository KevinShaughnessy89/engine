#pragma once
#include "core/PrecompiledHeader.hpp"

#include <cstdlib>

class MemoryPool {

    public:
        MemoryPool(size_t poolSize, size_t segmentSize);
        MemoryPool(const MemoryPool&) = delete;
        MemoryPool& operator=(const MemoryPool&) = delete;
        MemoryPool(MemoryPool&&) = delete;
        MemoryPool& operator=(const MemoryPool&&) = delete;
        ~MemoryPool() = default;

        void* allocate();
        void* deallocate(void* ptr);


    private:

        struct Segment {
            Segment* next;
        };

        void initializePool();
        std::byte* blockBegin;
        Segment* firstFreeSegment;
        size_t poolSize;
        size_t segmentSize;
        size_t nrSegements;
};
