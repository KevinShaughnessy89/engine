#include "ObjectForceGenerator.hpp"
#include "core/PrecompiledHeader.hpp"


// class SpringForceGenerator : ObjectForceGenerator {

//     protected:
//         PhysicsObject* other;
//         float restLength;
//         float springConstant;

//     public:
//         SpringForceGenerator( PhysicsObject* other, float restLength, float springConstant) :
//         other(other),
//                              restLength(restLength), springConstant(springConstant) {}

//         virtual void updateForce( PhysicsObject* object, float duration) {
//             glm::vec3 force  = object->getPosition();
//             force -= other->getPosition();

//             float magnitude = 0.0;
//             magnitude = abs(magnitude - restLength);
//             magnitude *= springConstant;

//             force.normalize();
//             force *= magnitude;
//             object->addForce(force)
//         }
// };
