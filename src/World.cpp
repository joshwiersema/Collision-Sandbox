#include "World.h"
#include <algorithm>
#include <random>
#include "Config.h"
#include "Parallel.h"

// Returns true if `candidate` overlaps any sphere already in `spheres`.
static bool overlapsAny(const Sphere& candidate, const std::vector<Sphere>& spheres) {
    for (const Sphere& existing : spheres) {
        if (spheresOverlap(candidate, existing)) {
            return true;
        }
    }
    return false;
}

World createRandomWorld(int sphereCount, float boxHalfSize, unsigned int seed) {
    World world;
    world.boxHalfSize = boxHalfSize;
    world.spheres.reserve(static_cast<size_t>(sphereCount));

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> radiusDist(Config::kMinRadius, Config::kMaxRadius);
    std::uniform_real_distribution<float> speedDist(-Config::kMaxSpawnSpeed, Config::kMaxSpawnSpeed);

    const int maxAttemptsPerSphere = 100;

    for (int i = 0; i < sphereCount; ++i) {
        Sphere sphere;
        sphere.radius = radiusDist(rng);
        sphere.mass = sphere.radius * sphere.radius * sphere.radius; // mass grows with volume
        sphere.velocity = Vec3(speedDist(rng), speedDist(rng), speedDist(rng));

        // Keep the whole sphere inside the box.
        const float range = boxHalfSize - sphere.radius;
        std::uniform_real_distribution<float> posDist(-range, range);

        // Try a few random spots until we find one that does not overlap.
        for (int attempt = 0; attempt < maxAttemptsPerSphere; ++attempt) {
            sphere.position = Vec3(posDist(rng), posDist(rng), posDist(rng));
            if (!overlapsAny(sphere, world.spheres)) {
                break;
            }
        }
        world.spheres.push_back(sphere);
    }
    return world;
}

void integrate(World& world, float timeStep, int numThreads) {
    const Vec3 gravity(0.0f, Config::kGravity, 0.0f);
    const int count = static_cast<int>(world.spheres.size());

    // Each thread owns a slice of the sphere array, so there is no sharing.
    parallelFor(count, numThreads, [&](int /*threadIndex*/, int begin, int end) {
        for (int i = begin; i < end; ++i) {
            Sphere& sphere = world.spheres[static_cast<size_t>(i)];

            // Semi-implicit Euler: update velocity first, then use the NEW
            // velocity to move. More stable than plain Euler for games.
            sphere.velocity += gravity * timeStep;
            sphere.position += sphere.velocity * timeStep;

            resolveWallCollision(sphere, world.boxHalfSize, Config::kRestitution);
        }
    });
}

void resolveCollisions(World& world, std::vector<CollisionPair> pairs, float restitution) {
    // Threads return pairs in whatever order they finish. Sorting makes the
    // result identical no matter how many threads were used (deterministic).
    std::sort(pairs.begin(), pairs.end(), [](const CollisionPair& lhs, const CollisionPair& rhs) {
        if (lhs.a != rhs.a) {
            return lhs.a < rhs.a;
        }
        return lhs.b < rhs.b;
    });

    for (const CollisionPair& pair : pairs) {
        Sphere& a = world.spheres[static_cast<size_t>(pair.a)];
        Sphere& b = world.spheres[static_cast<size_t>(pair.b)];
        resolveSphereCollision(a, b, restitution);
    }
}
