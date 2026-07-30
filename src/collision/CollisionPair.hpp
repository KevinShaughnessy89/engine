#include "collision/CollisionAliases.hpp"

struct CollisionPair {
    CollisionResultPtr result;
    CollisionObject* obj1;
    CollisionObject* obj2;
    CollisionLevel level;
    int collisionDuration;

    // Constructors
    CollisionPair(CollisionResultPtr r, CollisionObject* o1, CollisionObject* o2, CollisionLevel l,
                  int duration);
    CollisionPair();

    // Declare the Rule of 5 functions
    CollisionPair(const CollisionPair& other);                 // Copy constructor
    CollisionPair& operator=(const CollisionPair& other);      // Copy assignment
    CollisionPair(CollisionPair&& other) noexcept;             // Move constructor
    CollisionPair& operator=(CollisionPair&& other) noexcept;  // Move assignment
    ~CollisionPair();                                          // Destructor
};

CollisionPair::CollisionPair(CollisionResultPtr r, CollisionObject* o1, CollisionObject* o2,
                             CollisionLevel l, int duration)
    : result(r), obj1(o1), obj2(o2), level(l), collisionDuration(duration) {
}

CollisionPair::CollisionPair()
    : result(nullptr),
      obj1(nullptr),
      obj2(nullptr),
      level(CollisionLevel::TWO),
      collisionDuration(0) {
}

CollisionPair::CollisionPair(CollisionPair&& other) noexcept
    : result(other.result),
      obj1(other.obj1),
      obj2(other.obj2),
      level(other.level),
      collisionDuration(other.collisionDuration) {
    other.result = nullptr;  // Prevent double deletion
    other.obj1 = nullptr;
    other.obj2 = nullptr;
}

CollisionPair& CollisionPair::operator=(CollisionPair&& other) noexcept {
    if (this != &other) {
        delete result;  // Clean up existing resource
        result = other.result;
        obj1 = other.obj1;
        obj2 = other.obj2;
        level = other.level;
        collisionDuration = other.collisionDuration;

        // Reset the moved-from object
        other.result = nullptr;
        other.obj1 = nullptr;
        other.obj2 = nullptr;
    }
    return *this;
}

// Copy constructor - creates new object as copy of existing one
CollisionPair::CollisionPair(const CollisionPair& other)
    : obj1(other.obj1),
      obj2(other.obj2),
      level(other.level),
      collisionDuration(other.collisionDuration) {
    // Deep copy the result - create new CollisionResult
    if (other.result != nullptr) {
        result = new CollisionResult(*other.result);
    } else {
        result = nullptr;
    }
}

CollisionPair& CollisionPair::operator=(const CollisionPair& other) {
    if (this != &other) {
        delete result;

        obj1 = other.obj1;
        obj2 = other.obj2;
        level = other.level;
        collisionDuration = other.collisionDuration;

        if (other.result != nullptr) {
            result = new CollisionResult(*other.result);
        } else {
            result = nullptr;
        }
    }
    return *this;
}

CollisionPair::~CollisionPair() {
    delete result;
}