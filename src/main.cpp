// Collision Sandbox
// -----------------
// Drops a few thousand spheres into a box and simulates them bouncing around.
// Compares two broadphase collision strategies (brute force vs. spatial grid),
// each run on one thread and on all threads, then runs the full simulation
// and writes PPM image frames to the out/ folder.
//
// Usage: collision_sandbox [sphereCount] [frameCount] [threadCount]

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "BruteForce.h"
#include "Config.h"
#include "Parallel.h"
#include "PpmWriter.h"
#include "SpatialGrid.h"
#include "Timer.h"
#include "World.h"

namespace {

struct Options {
    int sphereCount = Config::kDefaultSpheres;
    int frameCount = Config::kDefaultFrames;
    int threadCount = hardwareThreadCount();
};

// Reads an integer argument, falling back to `fallback` if it is missing or not positive.
int readPositiveInt(int argc, char** argv, int index, int fallback) {
    if (index >= argc) {
        return fallback;
    }
    const int value = std::atoi(argv[index]);
    if (value <= 0) {
        std::printf("Warning: argument %d ('%s') is not a positive number, using %d\n",
                    index, argv[index], fallback);
        return fallback;
    }
    return value;
}

Options parseOptions(int argc, char** argv) {
    Options options;
    options.sphereCount = readPositiveInt(argc, argv, 1, options.sphereCount);
    options.frameCount = readPositiveInt(argc, argv, 2, options.frameCount);
    options.threadCount = readPositiveInt(argc, argv, 3, options.threadCount);
    return options;
}

// Times one call of a broadphase and prints the result.
// Returns the number of pairs found so the caller can compare strategies.
size_t benchmarkBruteForce(const World& world, int threads) {
    Timer timer;
    const std::vector<CollisionPair> pairs = findPairsBruteForce(world.spheres, threads);
    const double ms = timer.elapsedMilliseconds();
    std::printf("  brute force, %2d thread(s): %8.2f ms, %zu pairs\n", threads, ms, pairs.size());
    return pairs.size();
}

size_t benchmarkGrid(const World& world, SpatialGrid& grid, int threads) {
    Timer timer;
    grid.build(world.spheres);
    const std::vector<CollisionPair> pairs = grid.findPairs(world.spheres, threads);
    const double ms = timer.elapsedMilliseconds();
    std::printf("  spatial grid, %2d thread(s): %8.2f ms, %zu pairs\n", threads, ms, pairs.size());
    return pairs.size();
}

void runBenchmark(const Options& options) {
    std::printf("\n=== Broadphase benchmark (%d spheres) ===\n", options.sphereCount);

    // Run a few frames first so the spheres have had a chance to collide.
    World world = createRandomWorld(options.sphereCount, Config::kBoxHalfSize, Config::kSeed);
    SpatialGrid grid(Config::kBoxHalfSize, Config::kCellSize);
    for (int frame = 0; frame < 30; ++frame) {
        integrate(world, Config::kTimeStep, options.threadCount);
        grid.build(world.spheres);
        resolveCollisions(world, grid.findPairs(world.spheres, options.threadCount), Config::kRestitution);
    }

    const size_t bruteSerial = benchmarkBruteForce(world, 1);
    const size_t bruteParallel = benchmarkBruteForce(world, options.threadCount);
    const size_t gridSerial = benchmarkGrid(world, grid, 1);
    const size_t gridParallel = benchmarkGrid(world, grid, options.threadCount);

    const bool allMatch = bruteSerial == bruteParallel && bruteSerial == gridSerial && bruteSerial == gridParallel;
    std::printf("  pair counts %s\n", allMatch ? "MATCH (grid agrees with brute force)" : "MISMATCH - bug!");
}

void runSimulation(const Options& options) {
    std::printf("\n=== Simulation (%d spheres, %d frames, %d threads) ===\n",
                options.sphereCount, options.frameCount, options.threadCount);

    World world = createRandomWorld(options.sphereCount, Config::kBoxHalfSize, Config::kSeed);
    SpatialGrid grid(Config::kBoxHalfSize, Config::kCellSize);

    double totalMs = 0.0;
    int framesWritten = 0;

    for (int frame = 0; frame < options.frameCount; ++frame) {
        Timer timer;

        integrate(world, Config::kTimeStep, options.threadCount);                       // parallel
        grid.build(world.spheres);                                                       // serial
        std::vector<CollisionPair> pairs = grid.findPairs(world.spheres, options.threadCount); // parallel
        resolveCollisions(world, pairs, Config::kRestitution);                          // serial

        totalMs += timer.elapsedMilliseconds();

        if (frame % Config::kFrameWriteInterval == 0) {
            char name[64];
            std::snprintf(name, sizeof(name), "out/frame_%04d.ppm", frame);
            if (writeFramePpm(world, name, Config::kImageSize)) {
                ++framesWritten;
            } else {
                std::printf("  Warning: could not write %s (does the out/ folder exist?)\n", name);
            }
        }
    }

    std::printf("  average frame time: %.3f ms\n", totalMs / options.frameCount);
    std::printf("  wrote %d image frames to out/\n", framesWritten);
}

} // namespace

int main(int argc, char** argv) {
    const Options options = parseOptions(argc, argv);
    std::printf("Collision Sandbox - hardware threads available: %d\n", hardwareThreadCount());

    runBenchmark(options);
    runSimulation(options);
    return 0;
}
