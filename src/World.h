#pragma once
#include <vector>
#include "Collision.h"
#include "Sphere.h"

// Everything being simulated: a list of spheres inside a cube-shaped box.
struct World {
    std::vector<Sphere> spheres;
    float boxHalfSize = 1.0f;
};

// Fill the box with randomly placed, non-overlapping spheres with random velocities.
World createRandomWorld(int sphereCount, float boxHalfSize, unsigned int seed);

// Kinematics step: apply gravity, move every sphere, bounce off the walls.
// Every sphere is independent here, so this runs in parallel.
void integrate(World& world, float timeStep, int numThreads);

// Resolve every pair found by the broadphase. Runs on one thread because
// one sphere can appear in many pairs, and two threads writing to the same
// sphere at once would be a data race.
void resolveCollisions(World& world, std::vector<CollisionPair> pairs, float restitution);
