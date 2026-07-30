#pragma once
#include "core/PrecompiledHeader.hpp"

#include <mutex>
#include <iostream>
#include <chrono>

inline std::mutex cout_mutex;

#ifdef ENABLE_LOGGING

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#define CONCAT_IMPL(a,b) a##b
#define CONCAT(a,b) CONCAT_IMPL(a,b)

#define LOG(msg) \
    { \
        std::lock_guard<std::mutex> lock(cout_mutex); \
        std::cout << "[LOG] " << msg << " - " << __FILE__ << ":" << __LINE__ << std::endl; \
    }

#define LOG_SKIP(msg, seconds) \
    do { \
        using clock = std::chrono::steady_clock; \
        static auto CONCAT(_last_print_time_, __LINE__) = clock::now() - std::chrono::duration<double>(seconds); \
        auto now = clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(now - CONCAT(_last_print_time_, __LINE__)).count(); \
        if (duration >= (seconds)) { \
            CONCAT(_last_print_time_, __LINE__) = now; \
            std::lock_guard<std::mutex> lock(cout_mutex); \
            std::cout << "[LOG] " << msg << " - " << __FILE__ << ":" << __LINE__ << std::endl; \
        } \
    } while (0)
#else
    #define LOG(msg)
    #define LOG_SKIP(msg, seconds)
#endif
