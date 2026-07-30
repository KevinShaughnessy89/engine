#pragma once

class CollisionObject;
struct ContactManifold; 

struct CollisionEvent {
    CollisionObject* obj1;
    CollisionObject* obj2;
    ContactManifold* result;

    // Constructors
    CollisionEvent(CollisionObject* o1, CollisionObject* o2, ContactManifold* result) :
        obj1(o1), obj2(o2), result(result) {}
    CollisionEvent() : obj1(nullptr), obj2(nullptr), result(nullptr) {};                                     
}
