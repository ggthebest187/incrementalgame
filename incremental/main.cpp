#include <windows.h>
#include <gdiplus.h>
#include "game.h"
#include "ui.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Global state
GameState g_game;
UIManager g_ui;

// Timing
LARGE_INTEGER g_frequency;
LARGE_INTEGER g_lastTime;
LARGE_INTEGER g_fpsTime;
int g_fps = 0;
int g_frameCount = 0;

// Initialize timing
void InitTiming() {
    QueryPerformanceFrequency(&g_frequency);
    QueryPerformanceCounter(&g_lastTime);
    g_fpsTime = g_lastTime;
}

// Calculate delta time
float GetDeltaTime() {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    float deltaTime = (float)(currentTime.QuadPart - g_lastTime.QuadPart) / (float)g_frequency.QuadPart;
    g_lastTime = currentTime;

    // FPS calculation
    g_frameCount++;
    float fpsElapsed = (float)(currentTime.QuadPart - g_fpsTime.QuadPart) / (float)g_frequency.QuadPart;
    if (fpsElapsed >= 1.0f) {
        g_fps = g_frameCount;
        g_frameCount = 0;
        g_fpsTime = currentTime;
    }

    return deltaTime;
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        else {
            g_ui.HandleKeyPress(wParam, g_game);
        }
        return 0;

    case WM_MOUSEMOVE: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        g_ui.HandleMouseMove(x, y);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        g_ui.HandleMouseDown(x, y, g_game);
        return 0;
    }

    case WM_LBUTTONUP: {
        g_ui.HandleMouseUp();
        return 0;
    }

    case WM_RBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        g_ui.HandleRightMouseDown(x, y);
        return 0;
    }

    case WM_RBUTTONUP: {
        g_ui.HandleRightMouseUp();
        return 0;
    }

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        g_ui.HandleMouseWheel(delta);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Double buffering
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

        g_ui.Render(hdcMem, width, height, g_game, g_fps);

        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register window class
    const wchar_t CLASS_NAME[] = L"ProceduralCivWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Procedural Civilization - Idle Game",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 700,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Initialize game and UI
    InitTiming();
    g_ui.Initialize(g_game);

    // Main game loop
    MSG msg = {};
    bool running = true;

    while (running) {
        // Process messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (running) {
            // Update game
            float deltaTime = GetDeltaTime();
            g_game.Update(deltaTime);
            g_ui.Update(deltaTime, g_game);

            // Render
            InvalidateRect(hwnd, NULL, FALSE);

            // Sleep to limit frame rate to ~60 FPS
            Sleep(1);
        }
    }

    // Cleanup
    GdiplusShutdown(gdiplusToken);

    return 0;
}