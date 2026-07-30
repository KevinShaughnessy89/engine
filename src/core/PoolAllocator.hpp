#pragma once
#include "core/PrecompiledHeader.hpp"

template <typename T>
class PoolAllocator {
    public:

        PoolAllocator(MemoryPool& pool) : memoryPool(pool) {}

        T* allocate(std::size_t n) {
            void* mem = memoryPool.allocate();
            if (!mem) throw std::bad_alloc();
            return static_cast<T*>(mem);
        }

        void deallocate(T* p, std::size_t n) noexcept {
            memoryPool.deallocate(p);
        }

        MemoryPool& memoryPool;
};
