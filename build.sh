#!/bin/sh
# Builds the sandbox and the tests with g++ or clang++.
# The sandbox opens a Win32 + OpenGL window, so it needs Windows (MinGW).
# The tests have no window and build anywhere (Linux / macOS too).
set -e
mkdir -p build

CXX="${CXX:-g++}"
FLAGS="-std=c++17 -O2 -Wall -Wextra -pthread"
SOURCES="src/Collision.cpp src/BruteForce.cpp src/SpatialGrid.cpp src/World.cpp"
GL_LIBS="-lopengl32 -lgdi32 -luser32"

echo "Building tests ..."
$CXX $FLAGS tests/test_main.cpp $SOURCES -o build/tests

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        echo "Building collision_sandbox ..."
        $CXX $FLAGS src/main.cpp src/Renderer.cpp $SOURCES $GL_LIBS -o build/collision_sandbox
        echo "Done. Run ./build/tests then ./build/collision_sandbox"
        ;;
    *)
        echo "Skipping collision_sandbox: the window code is Windows-only. Run ./build/tests"
        ;;
esac
