#pragma once

#include "core/PrecompiledHeader.hpp"
#include "core/Subscription.hpp"
#include "core/Event.hpp"
#include "core/EventType.hpp"

class EventManager {
public:
    void subscribe(EventType type, std::function<void(const Event&)> callback) {
        subscribers[type].emplace_back(Subscription(std::move(callback)));
    }

    void publish(const Event& event) {
        for (const auto& sub : subscribers[event.type]) {
            sub.invoke(event);
        }
    }

private:
    std::unordered_map<EventType, std::vector<Subscription>> subscribers;
};
