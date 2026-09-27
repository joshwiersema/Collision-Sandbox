#pragma once
#include <string>
#include "World.h"

// Draws a front view (X across, Y up) of the world into a PPM image file.
// PPM is a plain text image format, so no image library is needed.
// Spheres are coloured by speed: blue = slow, red = fast.
// Returns false if the file could not be written.
bool writeFramePpm(const World& world, const std::string& path, int imageSize);
