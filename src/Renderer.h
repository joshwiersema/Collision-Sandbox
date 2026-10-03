#pragma once
#include "World.h"

// A live OpenGL window that shows a front view of the world (X across, Y up).
// Spheres are coloured by speed: blue = slow, red = fast.
// Uses only Win32 and the OpenGL 1.1 that ships with Windows, so no libraries are needed.
// There is only ever one window, so these are plain functions instead of a class.

// Opens a square window of `size` x `size` pixels. Returns false if it could not be created.
bool openWindow(int size, const char* title);

// Handles window messages (close button, Escape key).
// Returns false once the user has closed the window.
bool updateWindow();

// Clears the window, draws every sphere as a filled circle, and shows the result.
void drawWorld(const World& world);

// Releases the OpenGL context and closes the window. Safe to call more than once.
void closeWindow();
