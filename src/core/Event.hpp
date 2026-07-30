#pragma once
#include "EventType.hpp"
#include "collision/ContactManifold.hpp"
#include "core/PrecompiledHeader.hpp"


using EventData = std::variant<int>;

class Event {
   public:
    EventType type;
    std::vector<EventData> data;

    Event(EventType t, EventData data) : type(t), data({data}) {}
    bool operator==(const Event& other) { return type == type; }
};