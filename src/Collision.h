#pragma once
#include "Sphere.h"

// A pair of sphere indices that might be, or are, colliding.
struct CollisionPair {
    int a = 0;
    int b = 0;
};

// Narrowphase test: do the two spheres overlap?
bool spheresOverlap(const Sphere& a, const Sphere& b);

// Push the spheres apart and apply an impulse so they bounce.
void resolveSphereCollision(Sphere& a, Sphere& b, float restitution);

// Keep a sphere inside the box [-halfSize, +halfSize] on every axis, bouncing off walls.
void resolveWallCollision(Sphere& sphere, float halfSize, float restitution);
