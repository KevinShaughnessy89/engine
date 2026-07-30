#pragma once
#include "EventType.hpp"
#include "core/PrecompiledHeader.hpp"


class Event;

class Subscription {
   public:
    std::function<void(const Event&)> callback;

    // move semantics? Might introduce an error, maybe
    Subscription(std::function<void(const Event&)> cb) : callback(std::move(cb)) {}

    void invoke(const Event& event) const { callback(event); }
};
