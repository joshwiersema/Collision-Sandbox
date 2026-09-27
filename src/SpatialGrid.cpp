#include "SpatialGrid.h"
#include <cmath>
#include "Parallel.h"

SpatialGrid::SpatialGrid(float boxHalfSize, float cellSize)
    : boxHalfSize_(boxHalfSize), cellSize_(cellSize) {
    const float boxSize = 2.0f * boxHalfSize;
    cellsPerAxis_ = static_cast<int>(std::ceil(boxSize / cellSize));
    cells_.resize(static_cast<size_t>(cellCount()));
}

int SpatialGrid::cellCoordinate(float worldValue) const {
    int coord = static_cast<int>((worldValue + boxHalfSize_) / cellSize_);
    // Clamp so a sphere slightly outside the box still lands in an edge cell.
    if (coord < 0) coord = 0;
    if (coord >= cellsPerAxis_) coord = cellsPerAxis_ - 1;
    return coord;
}

int SpatialGrid::flatIndex(int cx, int cy, int cz) const {
    return (cz * cellsPerAxis_ + cy) * cellsPerAxis_ + cx;
}

void SpatialGrid::build(const std::vector<Sphere>& spheres) {
    for (std::vector<int>& cell : cells_) {
        cell.clear();
    }
    for (int i = 0; i < static_cast<int>(spheres.size()); ++i) {
        const Vec3& p = spheres[static_cast<size_t>(i)].position;
        const int index = flatIndex(cellCoordinate(p.x), cellCoordinate(p.y), cellCoordinate(p.z));
        cells_[static_cast<size_t>(index)].push_back(i);
    }
}

std::vector<CollisionPair> SpatialGrid::findPairs(const std::vector<Sphere>& spheres, int numThreads) const {
    std::vector<std::vector<CollisionPair>> perThreadPairs(static_cast<size_t>(numThreads));

    // Each thread takes a contiguous range of cells. build() is already done, so
    // cells_ is read-only here and threads never write to shared memory.
    parallelFor(cellCount(), numThreads, [&](int threadIndex, int begin, int end) {
        std::vector<CollisionPair>& localPairs = perThreadPairs[static_cast<size_t>(threadIndex)];

        for (int cellIndex = begin; cellIndex < end; ++cellIndex) {
            const std::vector<int>& cell = cells_[static_cast<size_t>(cellIndex)];
            if (cell.empty()) {
                continue;
            }

            // Recover the 3D cell coordinates from the flat index.
            const int cx = cellIndex % cellsPerAxis_;
            const int cy = (cellIndex / cellsPerAxis_) % cellsPerAxis_;
            const int cz = cellIndex / (cellsPerAxis_ * cellsPerAxis_);

            // Visit this cell and all 26 neighbours.
            for (int dz = -1; dz <= 1; ++dz) {
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = cx + dx;
                        const int ny = cy + dy;
                        const int nz = cz + dz;
                        const bool outside = nx < 0 || ny < 0 || nz < 0 ||
                                             nx >= cellsPerAxis_ || ny >= cellsPerAxis_ || nz >= cellsPerAxis_;
                        if (outside) {
                            continue;
                        }

                        const std::vector<int>& neighbour = cells_[static_cast<size_t>(flatIndex(nx, ny, nz))];
                        for (int i : cell) {
                            for (int j : neighbour) {
                                // Only accept i < j so each pair is reported exactly once.
                                // The pair is found when visiting the cell of i, and skipped
                                // when visiting the cell of j.
                                if (i < j && spheresOverlap(spheres[static_cast<size_t>(i)],
                                                            spheres[static_cast<size_t>(j)])) {
                                    localPairs.push_back(CollisionPair{i, j});
                                }
                            }
                        }
                    }
                }
            }
        }
    });

    std::vector<CollisionPair> allPairs;
    for (const std::vector<CollisionPair>& localPairs : perThreadPairs) {
        allPairs.insert(allPairs.end(), localPairs.begin(), localPairs.end());
    }
    return allPairs;
}
