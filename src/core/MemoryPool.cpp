#include "core/PrecompiledHeader.hpp"
#include "MemoryPool.hpp"

MemoryPool::MemoryPool(size_t poolSize, size_t segmentSize)
    :   poolSize(poolSize), segmentSize(segmentSize),
        blockBegin(nullptr), firstFreeSegment(nullptr),
        nrSegements(0)
{
    if (poolSize < sizeof(Segment)) {
        std::cerr << "Memory Pool - Pool size must be at least sizeof(Segment), or this will cause problems" << std::endl;;
    }

    if (segmentSize > poolSize) {
        std::cerr << "Memory Pool - Pool size must be greater than segment size, or this will cause problems" << std::endl;;
    }

    initializePool();
}

void MemoryPool::initializePool() 
{
    blockBegin = static_cast<std::byte*>(
        ::operator new(poolSize, std::align_val_t(alignof(std::max_align_t)))
    );    
    
    if (blockBegin == nullptr) {
        std::cerr << "Memory Pool - Error in creating blockBegin" << std::endl;
    }

    nrSegements = poolSize / segmentSize;

    firstFreeSegment = reinterpret_cast<Segment*>(blockBegin);

    Segment* current = firstFreeSegment;
    for (size_t i = 0; i < nrSegements; i++) {
        current->next = reinterpret_cast<Segment*>(reinterpret_cast<std::byte*>(current) + segmentSize);
        current = current->next;
    }

    current->next = nullptr;
}

void* MemoryPool::allocate()
{
    if (firstFreeSegment == nullptr) {
        std::cerr << "Memory Pool - no more segements in the pool" << std::endl;
        return nullptr;
    }

    void* memoryAlloc = firstFreeSegment;

    // increment firstFreeSegment by segmentSize
    firstFreeSegment = firstFreeSegment->next;

    return memoryAlloc;
}

void* deallocate(void* ptr) 
{
    if (ptr == nullptr) return NULL;


}
