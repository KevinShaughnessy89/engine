#pragma once
#include "core/PrecompiledHeader.hpp"



template <typename T, size_t initialSize = 32>
class VectorPool {
    private:
        std::vector<std::vector<T>> vectorPool;
    public:
        VectorPool() { vectorPool.reserve(initialSize); }

        std::vector<T>* get() {
            if (vectorPool.empty()) {
                return new std::vector<T>();
            }
            std::vector<T>* vec = &vectorPool.back();
            vectorPool.pop_back();
            return vec;
        }

        bool reclaim(std::vector<T>* vec) {
            if (vec == nullptr) return false;
            vec->clear();
            vectorPool.push_back(std::move(vec));
            vec = nullptr;
            return true;
        }    
}
