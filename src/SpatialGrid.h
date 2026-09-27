#pragma once
#include <vector>
#include "Collision.h"
#include "Sphere.h"

// A uniform grid broadphase. The box is divided into equal cubic cells.
// Each sphere is put in the cell its centre lies in. Two spheres can only
// touch if they are in the same cell or in neighbouring cells, so we only
// test those, instead of every possible pair.
class SpatialGrid {
public:
    SpatialGrid(float boxHalfSize, float cellSize);

    // Sort all spheres into cells. Must be called every frame before findPairs.
    void build(const std::vector<Sphere>& spheres);

    // Find every overlapping pair using the cells filled in by build().
    std::vector<CollisionPair> findPairs(const std::vector<Sphere>& spheres, int numThreads) const;

    int cellCount() const { return cellsPerAxis_ * cellsPerAxis_ * cellsPerAxis_; }

private:
    int cellCoordinate(float worldValue) const;   // world position -> cell index on one axis
    int flatIndex(int cx, int cy, int cz) const;  // 3 cell coordinates -> index into cells_

    float boxHalfSize_;
    float cellSize_;
    int cellsPerAxis_;
    std::vector<std::vector<int>> cells_; // cells_[cellIndex] = list of sphere indices in that cell
};
