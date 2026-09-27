// Simple test runner with no framework: each test is a function that calls
// check(). The program exits non-zero if any check fails.

#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/BruteForce.h"
#include "../src/Collision.h"
#include "../src/Config.h"
#include "../src/SpatialGrid.h"
#include "../src/Vec3.h"
#include "../src/World.h"

static int g_failures = 0;
static int g_checks = 0;

static void check(bool condition, const char* description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::printf("  FAIL: %s\n", description);
    }
}

static bool nearlyEqual(float a, float b, float tolerance = 1e-4f) {
    return std::fabs(a - b) <= tolerance;
}

static void testVec3() {
    std::printf("Vec3\n");
    const Vec3 a(1.0f, 2.0f, 3.0f);
    const Vec3 b(4.0f, 5.0f, 6.0f);

    check(dot(a, b) == 32.0f, "dot product");
    check(nearlyEqual(length(Vec3(3.0f, 4.0f, 0.0f)), 5.0f), "length of 3-4-5 triangle");
    check(nearlyEqual(length(normalized(b)), 1.0f), "normalized has length 1");
    check(length(normalized(Vec3())) == 0.0f, "normalizing zero vector gives zero");

    const Vec3 c = cross(Vec3(1, 0, 0), Vec3(0, 1, 0));
    check(c.x == 0.0f && c.y == 0.0f && c.z == 1.0f, "x cross y = z");
}

static void testSphereOverlap() {
    std::printf("spheresOverlap\n");
    Sphere a;
    a.position = Vec3(0, 0, 0);
    a.radius = 1.0f;
    Sphere b;
    b.position = Vec3(1.5f, 0, 0);
    b.radius = 1.0f;

    check(spheresOverlap(a, b), "spheres 1.5 apart with radius 1 overlap");
    b.position = Vec3(2.5f, 0, 0);
    check(!spheresOverlap(a, b), "spheres 2.5 apart with radius 1 do not overlap");
    b.position = Vec3(2.0f, 0, 0);
    check(!spheresOverlap(a, b), "spheres exactly touching do not count as overlapping");
}

static void testResolveConservesMomentum() {
    std::printf("resolveSphereCollision\n");
    Sphere a;
    a.position = Vec3(0, 0, 0);
    a.velocity = Vec3(5, 0, 0);
    a.radius = 1.0f;
    a.mass = 2.0f;
    Sphere b;
    b.position = Vec3(1.5f, 0, 0);
    b.velocity = Vec3(-1, 0, 0);
    b.radius = 1.0f;
    b.mass = 1.0f;

    const Vec3 momentumBefore = a.velocity * a.mass + b.velocity * b.mass;
    resolveSphereCollision(a, b, 1.0f);
    const Vec3 momentumAfter = a.velocity * a.mass + b.velocity * b.mass;

    check(nearlyEqual(momentumBefore.x, momentumAfter.x), "momentum conserved along x");
    check(!spheresOverlap(a, b), "spheres pushed apart after resolve");
    check(b.velocity.x > a.velocity.x, "spheres now moving apart");

    // Fully elastic head-on collision of equal masses swaps velocities.
    Sphere c;
    c.position = Vec3(0, 0, 0);
    c.velocity = Vec3(3, 0, 0);
    Sphere d;
    d.position = Vec3(1.9f, 0, 0);
    d.velocity = Vec3(0, 0, 0);
    resolveSphereCollision(c, d, 1.0f);
    check(nearlyEqual(c.velocity.x, 0.0f) && nearlyEqual(d.velocity.x, 3.0f),
          "equal mass elastic collision swaps velocities");
}

static void testWallCollision() {
    std::printf("resolveWallCollision\n");
    Sphere s;
    s.position = Vec3(0, -12.0f, 0);
    s.velocity = Vec3(0, -4.0f, 0);
    s.radius = 1.0f;

    resolveWallCollision(s, 10.0f, 0.5f);
    check(nearlyEqual(s.position.y, -9.0f), "sphere clamped to floor");
    check(nearlyEqual(s.velocity.y, 2.0f), "velocity flipped and scaled by restitution");
}

static void testGridMatchesBruteForce() {
    std::printf("SpatialGrid vs BruteForce\n");
    const World world = createRandomWorld(500, Config::kBoxHalfSize, 42u);

    // Move spheres a bit so some overlap.
    World moved = world;
    for (Sphere& s : moved.spheres) {
        s.position += s.velocity * 0.5f;
        resolveWallCollision(s, moved.boxHalfSize, 1.0f);
    }

    const std::vector<CollisionPair> brute1 = findPairsBruteForce(moved.spheres, 1);
    const std::vector<CollisionPair> brute4 = findPairsBruteForce(moved.spheres, 4);
    SpatialGrid grid(Config::kBoxHalfSize, Config::kCellSize);
    grid.build(moved.spheres);
    const std::vector<CollisionPair> grid1 = grid.findPairs(moved.spheres, 1);
    const std::vector<CollisionPair> grid4 = grid.findPairs(moved.spheres, 4);

    check(brute1.size() == brute4.size(), "brute force pair count same on 1 and 4 threads");
    check(brute1.size() == grid1.size(), "grid finds same number of pairs as brute force");
    check(grid1.size() == grid4.size(), "grid pair count same on 1 and 4 threads");
}

static void testSimulationIsDeterministic() {
    std::printf("Deterministic across thread counts\n");
    World worldA = createRandomWorld(300, Config::kBoxHalfSize, 7u);
    World worldB = worldA;
    SpatialGrid gridA(Config::kBoxHalfSize, Config::kCellSize);
    SpatialGrid gridB(Config::kBoxHalfSize, Config::kCellSize);

    for (int frame = 0; frame < 20; ++frame) {
        integrate(worldA, Config::kTimeStep, 1);
        gridA.build(worldA.spheres);
        resolveCollisions(worldA, gridA.findPairs(worldA.spheres, 1), Config::kRestitution);

        integrate(worldB, Config::kTimeStep, 4);
        gridB.build(worldB.spheres);
        resolveCollisions(worldB, gridB.findPairs(worldB.spheres, 4), Config::kRestitution);
    }

    bool identical = true;
    for (size_t i = 0; i < worldA.spheres.size(); ++i) {
        const Vec3 diff = worldA.spheres[i].position - worldB.spheres[i].position;
        if (lengthSquared(diff) != 0.0f) {
            identical = false;
            break;
        }
    }
    check(identical, "1-thread and 4-thread simulations produce identical positions");
}

int main() {
    testVec3();
    testSphereOverlap();
    testResolveConservesMomentum();
    testWallCollision();
    testGridMatchesBruteForce();
    testSimulationIsDeterministic();

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
