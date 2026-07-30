#pragma once
#include "core/PrecompiledHeader.hpp"

struct PairHash_1 {
    std::size_t operator()(const glm::ivec2& p) const {
        std::size_t seed = 0;
        seed ^= std::hash<int>{}(p.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct PairEqual_1 {
    bool operator()(const glm::ivec2& a, const glm::ivec2& b) const {
        return a.x == b.x && a.y == b.y;
    }
};

struct PairHash_typeindex_stdPair {
    size_t operator()(const std::pair<std::type_index, std::type_index>& p) const {
        return p.first.hash_code() ^ (p.second.hash_code() << 1);
    }
};