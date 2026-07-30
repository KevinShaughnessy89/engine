#include "core/PrecompiledHeader.hpp"

template<typename T>
class VectorPool {
    std::queue<std::unique_ptr<std::vector<T>>> available;
    size_t expectedSize;
    
public:
    SimpleVectorPool(size_t expectedSize) : expectedSize(expectedSize) {}
    
    std::unique_ptr<std::vector<T>> get() {
        if (available.empty()) {
            auto vec = std::make_unique<std::vector<T>>();
            vec->reserve(expectedSize);
            return vec;
        }
        auto vec = std::move(available.front());
        available.pop();
        vec->clear(); // Keeps capacity
        return vec;
    }
    
    void release(std::unique_ptr<std::vector<T>> vec) {
        available.push(std::move(vec));
    }
};
