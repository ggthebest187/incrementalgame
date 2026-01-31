#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Game state
struct GameState {
    double food = 0.0;
    double wood = 0.0;
    double stone = 0.0;
    double gold = 0.0;

    double foodPerSecond = 1.0;
    double woodPerSecond = 0.5;
    double stonePerSecond = 0.3;
    double goldPerSecond = 0.1;

    float gameTime = 0.0f;
    LARGE_INTEGER lastTime;
    LARGE_INTEGER frequency;

    int fps = 0;
    int frameCount = 0;
    LARGE_INTEGER fpsTime;
};

GameState g_game;

// Initialize timing
void InitTiming() {
    QueryPerformanceFrequency(&g_game.frequency);
    QueryPerformanceCounter(&g_game.lastTime);
    g_game.fpsTime = g_game.lastTime;
}

// Calculate delta time
float GetDeltaTime() {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    float deltaTime = (float)(currentTime.QuadPart - g_game.lastTime.QuadPart) / (float)g_game.frequency.QuadPart;
    g_game.lastTime = currentTime;

    // FPS calculation
    g_game.frameCount++;
    float fpsElapsed = (float)(currentTime.QuadPart - g_game.fpsTime.QuadPart) / (float)g_game.frequency.QuadPart;
    if (fpsElapsed >= 1.0f) {
        g_game.fps = g_game.frameCount;
        g_game.frameCount = 0;
        g_game.fpsTime = currentTime;
    }

    return deltaTime;
}

// Update game logic
void UpdateGame(float deltaTime) {
    g_game.gameTime += deltaTime;
    g_game.food += g_game.foodPerSecond * deltaTime;
    g_game.wood += g_game.woodPerSecond * deltaTime;
    g_game.stone += g_game.stonePerSecond * deltaTime;
    g_game.gold += g_game.goldPerSecond * deltaTime;
}

// Render the game
void RenderGame(HDC hdc, int width, int height) {
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

    // Clear background
    SolidBrush bgBrush(Color(255, 20, 20, 30));
    graphics.FillRectangle(&bgBrush, 0, 0, width, height);

    // Set up fonts
    FontFamily fontFamily(L"Arial");
    Font titleFont(&fontFamily, 24, FontStyleBold, UnitPixel);
    Font resourceFont(&fontFamily, 18, FontStyleRegular, UnitPixel);
    Font smallFont(&fontFamily, 14, FontStyleRegular, UnitPixel);

    // Title
    SolidBrush goldBrush(Color(255, 255, 215, 0));
    PointF titlePos(300.0f, 20.0f);
    graphics.DrawString(L"=== PROCEDURAL CIVILIZATION ===", -1, &titleFont, titlePos, &goldBrush);

    // FPS counter
    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    std::wstringstream fpsStream;
    fpsStream << L"FPS: " << g_game.fps;
    PointF fpsPos(10.0f, 10.0f);
    graphics.DrawString(fpsStream.str().c_str(), -1, &smallFont, fpsPos, &whiteBrush);

    // Resources
    SolidBrush foodBrush(Color(255, 100, 255, 100));
    SolidBrush woodBrush(Color(255, 139, 69, 19));
    SolidBrush stoneBrush(Color(255, 128, 128, 128));
    SolidBrush goldResourceBrush(Color(255, 255, 215, 0));

    float yPos = 100.0f;
    float xPos = 50.0f;

    // Food
    std::wstringstream foodStream;
    foodStream << L"Food: " << std::fixed << std::setprecision(1) << g_game.food;
    PointF foodPos(xPos, yPos);
    graphics.DrawString(foodStream.str().c_str(), -1, &resourceFont, foodPos, &foodBrush);
    yPos += 40.0f;

    // Wood
    std::wstringstream woodStream;
    woodStream << L"Wood: " << std::fixed << std::setprecision(1) << g_game.wood;
    PointF woodPos(xPos, yPos);
    graphics.DrawString(woodStream.str().c_str(), -1, &resourceFont, woodPos, &woodBrush);
    yPos += 40.0f;

    // Stone
    std::wstringstream stoneStream;
    stoneStream << L"Stone: " << std::fixed << std::setprecision(1) << g_game.stone;
    PointF stonePos(xPos, yPos);
    graphics.DrawString(stoneStream.str().c_str(), -1, &resourceFont, stonePos, &stoneBrush);
    yPos += 40.0f;

    // Gold
    std::wstringstream goldStream;
    goldStream << L"Gold: " << std::fixed << std::setprecision(1) << g_game.gold;
    PointF goldPos(xPos, yPos);
    graphics.DrawString(goldStream.str().c_str(), -1, &resourceFont, goldPos, &goldResourceBrush);
    yPos += 60.0f;

    // Production rates
    SolidBrush grayBrush(Color(255, 200, 200, 200));
    std::wstringstream prodStream;
    prodStream << L"Production Rates:\n"
        << L"  Food: +" << std::fixed << std::setprecision(1) << g_game.foodPerSecond << L"/s\n"
        << L"  Wood: +" << g_game.woodPerSecond << L"/s\n"
        << L"  Stone: +" << g_game.stonePerSecond << L"/s\n"
        << L"  Gold: +" << g_game.goldPerSecond << L"/s";

    RectF prodRect(xPos, yPos, 400.0f, 150.0f);
    graphics.DrawString(prodStream.str().c_str(), -1, &smallFont, prodRect, NULL, &grayBrush);
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
        return 0;

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

        RenderGame(hdcMem, width, height);

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
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Initialize timing
    InitTiming();

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
            UpdateGame(deltaTime);

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