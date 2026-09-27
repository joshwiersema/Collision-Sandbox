#include "Collision.h"

bool spheresOverlap(const Sphere& a, const Sphere& b) {
    // Compare squared distances to avoid a square root.
    const float radiusSum = a.radius + b.radius;
    const float distSq = lengthSquared(a.position - b.position);
    return distSq < radiusSum * radiusSum;
}

void resolveSphereCollision(Sphere& a, Sphere& b, float restitution) {
    const Vec3 delta = b.position - a.position;
    const float dist = length(delta);
    const float radiusSum = a.radius + b.radius;

    if (dist >= radiusSum || dist < 1e-6f) {
        return; // Not touching, or exactly on top of each other (skip to avoid divide by zero)
    }

    // Collision normal points from a to b.
    const Vec3 normal = delta / dist;

    // 1. Positional correction: push the spheres apart so they no longer overlap.
    //    Split the push based on mass (heavier sphere moves less).
    const float penetration = radiusSum - dist;
    const float totalMass = a.mass + b.mass;
    a.position -= normal * (penetration * (b.mass / totalMass));
    b.position += normal * (penetration * (a.mass / totalMass));

    // 2. Impulse: change velocities along the normal.
    const Vec3 relativeVelocity = b.velocity - a.velocity;
    const float speedAlongNormal = dot(relativeVelocity, normal);

    if (speedAlongNormal > 0.0f) {
        return; // Already separating, no impulse needed
    }

    // Standard impulse formula for two spheres with restitution e:
    // j = -(1 + e) * v_rel_n / (1/m_a + 1/m_b)
    const float inverseMassSum = (1.0f / a.mass) + (1.0f / b.mass);
    const float impulseMagnitude = -(1.0f + restitution) * speedAlongNormal / inverseMassSum;
    const Vec3 impulse = normal * impulseMagnitude;

    a.velocity -= impulse / a.mass;
    b.velocity += impulse / b.mass;
}

// Helper: clamp one axis of the sphere against two walls.
static void bounceAxis(float& position, float& velocity, float radius,
                       float halfSize, float restitution) {
    const float minAllowed = -halfSize + radius;
    const float maxAllowed = halfSize - radius;

    if (position < minAllowed) {
        position = minAllowed;
        if (velocity < 0.0f) {
            velocity = -velocity * restitution;
        }
    } else if (position > maxAllowed) {
        position = maxAllowed;
        if (velocity > 0.0f) {
            velocity = -velocity * restitution;
        }
    }
}

void resolveWallCollision(Sphere& sphere, float halfSize, float restitution) {
    bounceAxis(sphere.position.x, sphere.velocity.x, sphere.radius, halfSize, restitution);
    bounceAxis(sphere.position.y, sphere.velocity.y, sphere.radius, halfSize, restitution);
    bounceAxis(sphere.position.z, sphere.velocity.z, sphere.radius, halfSize, restitution);
}
