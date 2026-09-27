#!/bin/sh
# Builds the sandbox and the tests with g++ or clang++ (Linux / macOS / MinGW).
set -e
mkdir -p build out

CXX="${CXX:-g++}"
FLAGS="-std=c++17 -O2 -Wall -Wextra -pthread"
SOURCES="src/Collision.cpp src/BruteForce.cpp src/SpatialGrid.cpp src/World.cpp src/PpmWriter.cpp"

echo "Building collision_sandbox ..."
$CXX $FLAGS src/main.cpp $SOURCES -o build/collision_sandbox

echo "Building tests ..."
$CXX $FLAGS tests/test_main.cpp $SOURCES -o build/tests

echo "Done. Run ./build/tests then ./build/collision_sandbox"
