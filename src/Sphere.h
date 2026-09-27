#pragma once
#include "Vec3.h"

// The only rigid body shape in this sandbox.
struct Sphere {
    Vec3 position;
    Vec3 velocity;
    float radius = 1.0f;
    float mass = 1.0f;
};
