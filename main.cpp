#include <windows.h>
#include <iostream>
#include <string>
#include "ascii_renderer.h"

// 全局变量
bool g_running = true;
const int ASCII_WIDTH = 150; // 增加宽度以获得更好的细节

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == 'Q') {
                g_running = false;
                PostQuitMessage(0);
            }
            return 0;
        case WM_ERASEBKGND:
            return 1; // 防止闪烁，自己绘制背景
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main() {
    std::cout << "Starting ASCII Screen Shader (Memory Mode)..." << std::endl;
    std::cout << "Initializing..." << std::endl;

    // 获取屏幕尺寸
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    // 注册窗口类
    const char CLASS_NAME[] = "ASCIIShaderWindow";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW, // 移除 WS_EX_LAYERED 以提高性能，不需要透明
        CLASS_NAME,
        "ASCII Shader",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenW, screenH,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (hwnd == NULL) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }

    // 初始化渲染器
    AsciiRenderer renderer;
    if (!renderer.Initialize(ASCII_WIDTH)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return 1;
    }

    HDC hWinDC = GetDC(hwnd);
    HDC hScreenDC = GetDC(NULL);

    // 双缓冲设置
    // 1. 用于捕获屏幕的 buffer
    HDC hCaptureDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hCaptureBmp = CreateCompatibleBitmap(hScreenDC, screenW, screenH);
    SelectObject(hCaptureDC, hCaptureBmp);

    // 2. 用于绘制 ASCII 的 buffer (Back Buffer)
    HDC hBackDC = CreateCompatibleDC(hWinDC);
    HBITMAP hBackBmp = CreateCompatibleBitmap(hWinDC, screenW, screenH);
    SelectObject(hBackDC, hBackBmp);

    std::cout << "Running. Press 'Q' to exit." << std::endl;

    MSG msg = { };
    while (g_running) {
        // 处理消息
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }

        if (!g_running) break;

        // 1. 捕获屏幕 (Hook/Capture)
        // 注意：为了避免捕获到我们自己的窗口（产生无限递归/镜像效果），
        // 我们可以先隐藏窗口，截屏，再显示。但这会闪烁。
        // 或者使用 PrintWindow (仅针对特定窗口)。
        // 但对于全局效果，递归视觉效果通常是可以接受的，甚至是有趣的。
        // 如果想避免，需要更高级的 DXGI 截屏（可能会忽略 Overlay）。
        // 这里直接 BitBlt，会包含所有内容。
        BitBlt(hCaptureDC, 0, 0, screenW, screenH, hScreenDC, 0, 0, SRCCOPY);

        // 2. 处理并绘制到 BackBuffer
        renderer.ProcessAndDraw(hCaptureDC, hBackDC, screenW, screenH);

        // 3. Present (BackBuffer -> Window)
        BitBlt(hWinDC, 0, 0, screenW, screenH, hBackDC, 0, 0, SRCCOPY);

        // 简单的帧率控制，避免占满 CPU
        Sleep(10);
    }

    // 清理资源
    DeleteDC(hCaptureDC);
    DeleteObject(hCaptureBmp);
    DeleteDC(hBackDC);
    DeleteObject(hBackBmp);
    ReleaseDC(NULL, hScreenDC);
    ReleaseDC(hwnd, hWinDC);

    return 0;
}
