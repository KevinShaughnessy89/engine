#pragma once
#include "core/PrecompiledHeader.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class FixedRateClock {
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;  // Use clock's native time_point
    using Duration = Clock::duration;     // Use clock's native duration
    using TickCallback = std::function<void(double fixedDeltaTime)>;

   public:
    FixedRateClock(int64_t rateHz)
        : running(false), fixedDeltaTime(static_cast<int64_t>(1e9) / rateHz), tickDuration(Duration(fixedDeltaTime)) {}

    void registerCallback(std::function<void(double fixedDeltaTime)> callback) {
        std::lock_guard lock(callbackMutex);
        callbacks.emplace_back(callback);
    }

    double getAlpha(double currentTime) {
        std::lock_guard lock(accumulatorMutex);
        double alpha = std::chrono::duration<double>(accumulator).count() /
                       std::chrono::duration<double>(tickDuration).count();
        return alpha;
        // return (currentTime - (double)lastUpdateTime) / (double)tickDuration.count();
    }

    void run() {
        TimePoint lastUpdateTime = Clock::now();
        accumulator = Duration::zero();

        while (running) {
            TimePoint now = Clock::now();
            Duration frameTime = now - lastUpdateTime;
            lastUpdateTime = now;

            if (frameTime.count() > 250'000'000) {  // 250ms max
                frameTime = Duration(250'000'000);
            }

            {
                std::lock_guard<std::mutex> lock(accumulatorMutex);
                accumulator += frameTime;
            }
            
            {
                std::lock_guard<std::mutex> lock(accumulatorMutex);
                while (accumulator >= tickDuration) {
                    tick(fixedDeltaTime);
                    accumulator -= tickDuration;
                }
            }
            
            TimePoint nextTickTime = lastUpdateTime + tickDuration - accumulator;
        }
    }

    void tick(int64_t deltaTime) {
        std::lock_guard lock(callbackMutex);
        for (const auto& callback : callbacks) {
            callback(static_cast<double>(deltaTime));
        }
    }

    void stop() {
        running = false;
        if (tickerThread.joinable()) tickerThread.join();
    }

    void start() {
        running = true;
        tickerThread = std::thread(&FixedRateClock::run, this);
    }

    ~FixedRateClock() { stop(); }

   private:
    std::atomic<bool> running;
    int64_t fixedDeltaTime;
    int64_t lastUpdateTime;
    Duration accumulator;
    Duration tickDuration;
    std::thread tickerThread;

    std::vector<TickCallback> callbacks;
    std::mutex callbackMutex;
    std::mutex accumulatorMutex;
};
