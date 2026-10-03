#include "Renderer.h"
#include <windows.h> // must come before gl.h
#include <GL/gl.h>
#include <cmath>
#include "Config.h"

namespace {

// Handles that Windows gives back to us. There is only one window, so they live here.
HWND gWindow = nullptr;        // the window itself
HDC gDeviceContext = nullptr;  // what we draw into
HGLRC gGlContext = nullptr;    // OpenGL's state for this window
bool gWindowOpen = false;

const char* const kWindowClassName = "CollisionSandboxWindow";
const float kMaxSpeedForColor = 30.0f;
const float kPi = 3.14159265f;

// Windows calls this whenever something happens to the window.
// We only care about the user closing it; everything else gets the default behaviour.
LRESULT CALLBACK handleWindowMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CLOSE) {
        gWindowOpen = false;
        return 0;
    }
    if (message == WM_KEYDOWN && wParam == VK_ESCAPE) {
        gWindowOpen = false;
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

// Turns on vsync so the window shows one frame per monitor refresh.
// The function is an OpenGL extension, so we have to ask the driver for it by name.
void enableVsync() {
    typedef BOOL(WINAPI * SwapIntervalFunction)(int);
    const SwapIntervalFunction setSwapInterval =
        reinterpret_cast<SwapIntervalFunction>(wglGetProcAddress("wglSwapIntervalEXT"));
    if (setSwapInterval != nullptr) {
        setSwapInterval(1);
    }
}

// Blend from blue (slow) to red (fast).
void setColorForSpeed(float speed) {
    float t = speed / kMaxSpeedForColor;
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;
    glColor3f(t, 60.0f / 255.0f, 1.0f - t);
}

// A filled circle is a "fan" of thin triangles that all share the centre point.
void drawCircle(float centerX, float centerY, float radius) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);
    for (int i = 0; i <= Config::kCircleSegments; ++i) {
        const float angle = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(Config::kCircleSegments);
        glVertex2f(centerX + radius * std::cos(angle), centerY + radius * std::sin(angle));
    }
    glEnd();
}

// Asks Windows for a pixel format OpenGL can draw into, then creates the OpenGL context.
bool createGlContext() {
    PIXELFORMATDESCRIPTOR format = {};
    format.nSize = sizeof(format);
    format.nVersion = 1;
    format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    format.iPixelType = PFD_TYPE_RGBA;
    format.cColorBits = 32;

    const int formatIndex = ChoosePixelFormat(gDeviceContext, &format);
    if (formatIndex == 0 || !SetPixelFormat(gDeviceContext, formatIndex, &format)) {
        return false;
    }

    gGlContext = wglCreateContext(gDeviceContext);
    if (gGlContext == nullptr) {
        return false;
    }
    return wglMakeCurrent(gDeviceContext, gGlContext) == TRUE;
}

} // namespace

bool openWindow(int size, const char* title) {
    const HINSTANCE instance = GetModuleHandleA(nullptr);

    // Tell Windows what kind of window we want and which function handles its messages.
    WNDCLASSA windowClass = {};
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = handleWindowMessage;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClassName;
    if (RegisterClassA(&windowClass) == 0) {
        return false;
    }

    // Grow the window so the drawable area (not counting the title bar) is size x size.
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
    RECT rect = {0, 0, size, size};
    AdjustWindowRect(&rect, style, FALSE);

    gWindow = CreateWindowA(kWindowClassName, title, style, CW_USEDEFAULT, CW_USEDEFAULT,
                            rect.right - rect.left, rect.bottom - rect.top,
                            nullptr, nullptr, instance, nullptr);
    if (gWindow == nullptr) {
        return false;
    }

    gDeviceContext = GetDC(gWindow);
    if (gDeviceContext == nullptr || !createGlContext()) {
        closeWindow();
        return false;
    }

    enableVsync();
    gWindowOpen = true;
    return true;
}

bool updateWindow() {
    MSG message;
    while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return gWindowOpen;
}

void drawWorld(const World& world) {
    glClearColor(20.0f / 255.0f, 20.0f / 255.0f, 25.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Map the box straight onto the window: [-halfSize, +halfSize] on both axes.
    // OpenGL's Y already points up, so unlike an image file nothing needs flipping.
    const double half = world.boxHalfSize;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-half, half, -half, half, -1.0, 1.0);

    for (const Sphere& sphere : world.spheres) {
        setColorForSpeed(length(sphere.velocity));
        drawCircle(sphere.position.x, sphere.position.y, sphere.radius);
    }

    // We drew into a hidden back buffer. Swapping shows the whole finished frame at once.
    SwapBuffers(gDeviceContext);
}

void closeWindow() {
    if (gGlContext != nullptr) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(gGlContext);
        gGlContext = nullptr;
    }
    if (gDeviceContext != nullptr) {
        ReleaseDC(gWindow, gDeviceContext);
        gDeviceContext = nullptr;
    }
    if (gWindow != nullptr) {
        DestroyWindow(gWindow);
        gWindow = nullptr;
    }
    gWindowOpen = false;
}
