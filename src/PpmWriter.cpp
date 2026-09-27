#include "PpmWriter.h"
#include <fstream>
#include <vector>

namespace {

struct Color {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
};

const float kMaxSpeedForColor = 30.0f;

// Blend from blue (slow) to red (fast).
Color colorForSpeed(float speed) {
    float t = speed / kMaxSpeedForColor;
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;
    Color color;
    color.r = static_cast<unsigned char>(255.0f * t);
    color.g = static_cast<unsigned char>(60.0f);
    color.b = static_cast<unsigned char>(255.0f * (1.0f - t));
    return color;
}

// Paints a filled circle into the pixel buffer, skipping pixels outside the image.
void drawCircle(std::vector<Color>& pixels, int imageSize, int centerX, int centerY, int radius, Color color) {
    for (int y = centerY - radius; y <= centerY + radius; ++y) {
        for (int x = centerX - radius; x <= centerX + radius; ++x) {
            const bool inside = x >= 0 && y >= 0 && x < imageSize && y < imageSize;
            if (!inside) {
                continue;
            }
            const int dx = x - centerX;
            const int dy = y - centerY;
            if (dx * dx + dy * dy <= radius * radius) {
                pixels[static_cast<size_t>(y * imageSize + x)] = color;
            }
        }
    }
}

} // namespace

bool writeFramePpm(const World& world, const std::string& path, int imageSize) {
    std::vector<Color> pixels(static_cast<size_t>(imageSize * imageSize), Color{20, 20, 25});

    // World units -> pixels. The box spans [-halfSize, +halfSize].
    const float pixelsPerUnit = static_cast<float>(imageSize) / (2.0f * world.boxHalfSize);

    for (const Sphere& sphere : world.spheres) {
        const int px = static_cast<int>((sphere.position.x + world.boxHalfSize) * pixelsPerUnit);
        // Flip Y so that "up" in the world is "up" in the picture.
        const int py = static_cast<int>((world.boxHalfSize - sphere.position.y) * pixelsPerUnit);
        int pr = static_cast<int>(sphere.radius * pixelsPerUnit);
        if (pr < 1) pr = 1;

        drawCircle(pixels, imageSize, px, py, pr, colorForSpeed(length(sphere.velocity)));
    }

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    // P6 header = binary RGB. "255" is the max colour value.
    file << "P6\n" << imageSize << " " << imageSize << "\n255\n";
    for (const Color& c : pixels) {
        file.put(static_cast<char>(c.r));
        file.put(static_cast<char>(c.g));
        file.put(static_cast<char>(c.b));
    }
    return file.good();
}
