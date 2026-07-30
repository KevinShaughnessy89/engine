#pragma once

struct CollisionResult;
class ContactManifold;
class CollisionShape;
class CollisionObject;
class CollisionGeometryCandidatePair;

using CollisionResultPtr = std::unique_ptr<CollisionResult>;
using ContactManifoldPtr = std::unique_ptr<ContactManifold>;
using SweepReturnVector = std::vector<std::pair<std::vector<CollisionGeometryCandidatePair>,
                                                std::vector<std::unique_ptr<CollisionObject>>>>;
using SweepReturnPair = std::pair<std::vector<CollisionGeometryCandidatePair>,
                                  std::vector<std::unique_ptr<CollisionObject>>>;
using CollisionShapePtr = std::unique_ptr<CollisionShape>;
using CollisionObjectPtr = std::unique_ptr<CollisionObject>;