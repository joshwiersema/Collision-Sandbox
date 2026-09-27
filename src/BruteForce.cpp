#include "BruteForce.h"
#include "Parallel.h"

std::vector<CollisionPair> findPairsBruteForce(const std::vector<Sphere>& spheres, int numThreads) {
    const int count = static_cast<int>(spheres.size());

    // Each thread writes into its own vector, so no locking is needed.
    std::vector<std::vector<CollisionPair>> perThreadPairs(static_cast<size_t>(numThreads));

    parallelFor(count, numThreads, [&](int threadIndex, int begin, int end) {
        std::vector<CollisionPair>& localPairs = perThreadPairs[static_cast<size_t>(threadIndex)];
        for (int i = begin; i < end; ++i) {
            for (int j = i + 1; j < count; ++j) {
                if (spheresOverlap(spheres[i], spheres[j])) {
                    localPairs.push_back(CollisionPair{i, j});
                }
            }
        }
    });

    // Merge the per-thread results into one list.
    std::vector<CollisionPair> allPairs;
    for (const std::vector<CollisionPair>& localPairs : perThreadPairs) {
        allPairs.insert(allPairs.end(), localPairs.begin(), localPairs.end());
    }
    return allPairs;
}
