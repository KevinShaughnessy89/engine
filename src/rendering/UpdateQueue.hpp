#pragma once
#include "core/PrecompiledHeader.hpp"

#include <mutex>
#include <queue>

template <typename T>
class UpdateQueue {
   private:
    std::queue<T> pendingUpdates;
    std::mutex pendingMutex;

    std::queue<T> completedUpdates;
    std::mutex completedMutex;

   public:
    // Enqueue a new update (replacing any existing one)
    void requestUpdate(const T& message) {
        std::lock_guard<std::mutex> lock(pendingMutex);
        if (!pendingUpdates.empty()) {
            pendingUpdates.pop();  // discard old
        }
        pendingUpdates.push(message);
    }

    // Retrieve and remove a pending update
    bool getPendingUpdate(T& message) {
        std::lock_guard<std::mutex> lock(pendingMutex);
        if (pendingUpdates.empty()) return false;

        message = pendingUpdates.front();
        pendingUpdates.pop();
        return true;
    }

    // Submit a completed update (replacing any existing one)
    void submitCompletedUpdate(const T& message) {
        std::lock_guard<std::mutex> lock(completedMutex);
        if (!completedUpdates.empty()) {
            completedUpdates.pop();  // discard old
        }
        completedUpdates.push(message);
    }

    // Retrieve and remove a completed update
    bool getCompletedUpdate(T& message) {
        std::lock_guard<std::mutex> lock(completedMutex);
        if (completedUpdates.empty()) return false;

        message = completedUpdates.front();
        completedUpdates.pop();
        return true;
    }

    bool noCompleted() {
        std::lock_guard<std::mutex> lock(completedMutex);
        return completedUpdates.empty();
    }
};
