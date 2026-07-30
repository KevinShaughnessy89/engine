#include "ContactManifold.hpp"

#include "core/PrecompiledHeader.hpp"

// bool ContactManifold::operator==(const ContactManifold& other) const {
//     return bodyA->UID == other.bodyA->UID && bodyB->UID == other.bodyB->UID &&
//            terrainTriangle == other.terrainTriangle;
// }

// ContactManifold& ContactManifold::operator=(ContactManifold&& other) noexcept {
//     if (this != &other) {
//         // Move each member
//         contacts = std::move(other.contacts);
//         numContacts = other.numContacts;
//         // maxContacts is const, so we don't move it
//         bodyA = other.bodyA;
//         bodyB = other.bodyB;
//         terrainTriangle = other.terrainTriangle;

//         // Reset the moved-from object
//         other.contacts.clear();
//         other.numContacts = 0;
//         other.bodyA = nullptr;
//         other.bodyB = nullptr;
//     }
//     return *this;
// }

// std::size_t ContactManifold::hash() const {
//     std::size_t h1 = std::hash<std::string>{}(bodyA->UID);
//     std::size_t h2 = std::hash<std::string>{}(bodyB->UID);
//     std::size_t h3 = std::hash<int>{}(terrainTriangle->indexA);
//     std::size_t h4 = std::hash<int>{}(terrainTriangle->indexB);
//     std::size_t h5 = std::hash<int>{}(terrainTriangle->indexC);

//     return h1 ^ (h2 << 1) ^ (h3 << 1) ^ (h4 << 1) ^ (h5 << 1);
// }

// bool ContactManifold::contactsStillValid(const ContactPoint& current) {
//     for (const auto& contact : contacts) {
//         if (*contact == current) return true;
//     }
//     return false;
// }

// void ContactManifold::update(const CollisionResultPtr result, ContactResolver contactResolver) {
//     ContactManifold newManifold;

//     volumeA->populateContactManifold(volumeB, newManifold, result, contactResolver);

//     contacts.erase(std::remove_if(contacts.begin(), contacts.end(),
//                                   [&newContacts = newManifold.contacts,
//                                    &numContacts = this->numContacts](ContactPoint* oldContact) {
//                                       auto it = std::find_if(
//                                           newContacts.begin(), newContacts.end(),
//                                           [&oldContact](const ContactPoint* newContact) {
//                                               return *oldContact == *newContact;
//                                           });
//                                       if (it != newContacts.end()) {
//                                           // Update existing contact
//                                           oldContact->position = (*it)->position;
//                                           oldContact->normal = (*it)->normal;
//                                           oldContact->penetrationDepth = (*it)->penetrationDepth;
//                                           oldContact->numFrames++;
//                                           // Mark this new contact as processed
//                                           newContacts.erase(it);
//                                           return false;  // Keep this contact
//                                       }
//                                       numContacts--;
//                                       return true;  // Remove this contact
//                                   }),
//                    contacts.end());

//     if (numContacts == 0 && newManifold.contacts.size() == 0) {
//         isSeperated = true;
//         return;
//     }

//     // Add remaining new contacts
//     for (const auto& newContact : newManifold.contacts) {
//         if (numContacts < maxContacts) {
//             contacts.push_back(newContact);
//             numContacts++;
//         }
//     }
// }
