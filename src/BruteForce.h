#pragma once
#include <vector>
#include "Collision.h"
#include "Sphere.h"

// The O(n^2) broadphase: test every sphere against every other sphere.
// Simple and always correct, used as the reference the grid is checked against.
std::vector<CollisionPair> findPairsBruteForce(const std::vector<Sphere>& spheres, int numThreads);
