#pragma once

// All tunable numbers in one place so nothing is hardcoded in the logic.
namespace Config {
    // World
    const float kBoxHalfSize   = 25.0f;   // Box goes from -25 to +25 on every axis
    const float kMinRadius     = 0.5f;
    const float kMaxRadius     = 1.5f;
    const float kGravity       = -9.8f;   // Along Y
    const float kRestitution   = 0.8f;    // 1 = perfectly bouncy, 0 = no bounce
    const float kMaxSpawnSpeed = 10.0f;

    // Simulation
    const float kTimeStep      = 1.0f / 60.0f;
    const int   kDefaultSpheres = 2000;
    const int   kDefaultFrames  = 120;
    const int   kFrameWriteInterval = 10; // Write a PPM image every N frames

    // Broadphase grid. Cell must be at least one full diameter so that
    // any two touching spheres are in the same or neighbouring cells.
    const float kCellSize = 2.0f * kMaxRadius;

    // Rendering
    const int kImageSize = 512;

    // Random seed so runs are repeatable
    const unsigned int kSeed = 12345u;
}
