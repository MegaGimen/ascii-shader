#include <windows.h>
#include <iostream>
#include <string>
#include "ascii_renderer.h"

// 全局变量
bool g_running = true;
int g_asciiWidth = 150; // 默认宽度

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;
        case WM_MOUSEACTIVATE:
            // 禁止鼠标激活窗口
            return MA_NOACTIVATE;
        case WM_NCHITTEST:
            // 确保所有点击都穿透 (虽然 WS_EX_TRANSPARENT 已经做了，但双重保险)
            return HTTRANSPARENT;
        // 注意：当窗口穿透鼠标时，WM_KEYDOWN 可能无法接收
        // 所以我们可能需要使用 GetAsyncKeyState 在主循环中检测
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main(int argc, char* argv[]) {
    // 启用 DPI 感知，确保获取真实的物理分辨率
    SetProcessDPIAware();

    if (argc > 1) {
        g_asciiWidth = std::stoi(argv[1]);
        if (g_asciiWidth < 10) g_asciiWidth = 10;
        // 移除 1000 的上限限制，允许用户设置为 1920 等高分辨率
        if (g_asciiWidth > 8000) g_asciiWidth = 8000; // 设置一个合理的上限
    }

    std::cout << "Starting ASCII Screen Shader (Memory Mode)..." << std::endl;
    std::cout << "Resolution (Width): " << g_asciiWidth << std::endl;
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
        // WS_EX_NOACTIVATE: 禁止窗口被激活
        // WS_EX_TRANSPARENT: 鼠标穿透
        // WS_EX_LAYERED: 必须配合使用
        // WS_EX_TOOLWINDOW: 不在任务栏显示
        // WS_EX_TOPMOST: 置顶
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE, 
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

    // 设置不透明度 (255 = 完全不透明) - 需要 WS_EX_LAYERED 才能使 WS_EX_TRANSPARENT 生效(在某些旧系统上)
    // 但更重要的是 WS_EX_TRANSPARENT 本身。
    // 为了保险，设置 Layered 属性
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    // 防止截屏递归 (Visual Feedback Loop)
    // WDA_EXCLUDEFROMCAPTURE (0x00000011) 仅在 Win10 2004+ 支持
    // 它允许窗口在截屏中不可见（透视），从而捕获到底层内容
    #ifndef WDA_EXCLUDEFROMCAPTURE
    #define WDA_EXCLUDEFROMCAPTURE 0x00000011
    #endif
    
    if (!SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE)) {
        std::cerr << "Warning: Failed to set Window Display Affinity. You may experience feedback loops." << std::endl;
        // 尝试旧版参数，虽然可能导致黑屏而不是透视
        SetWindowDisplayAffinity(hwnd, 0x01); // WDA_MONITOR
    }

    // 初始化渲染器
    AsciiRenderer renderer;
    if (!renderer.Initialize(g_asciiWidth)) {
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
        // 检测全局按键 'Q' (0x51)
        if (GetAsyncKeyState(0x51) & 0x8000) {
            g_running = false;
        }

        // 处理消息
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }

        if (!g_running) break;

        // 1. 捕获屏幕 (Hook/Capture)
        // BitBlt(hCaptureDC, 0, 0, screenW, screenH, hScreenDC, 0, 0, SRCCOPY);
        
        // 我们需要把 CaptureDC 的内容给 Renderer
        // 但 Renderer 现在接受 std::vector<RGBQUAD> 的小图
        // 所以我们在这里做缩放和读取像素
        
        // 创建一个小图 DC 用于缩放 (在 main 循环外创建更好，但为了最少改动...)
        // 实际上，AsciiRenderer 内部也可以处理，但现在接口变了
        // 我们在 main 里面做缩放比较灵活
        
        // 缩放：Screen -> Small
        static HDC hSmallDC = NULL;
        static HBITMAP hSmallBmp = NULL;
        static std::vector<RGBQUAD> pixels;
        static BITMAPINFO bmi = {0};
        
        if (!hSmallDC) {
            hSmallDC = CreateCompatibleDC(hScreenDC);
            hSmallBmp = CreateCompatibleBitmap(hScreenDC, renderer.GetWidth(), renderer.GetHeight());
            SelectObject(hSmallDC, hSmallBmp);
            
            pixels.resize(renderer.GetWidth() * renderer.GetHeight());
            
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = renderer.GetWidth();
            bmi.bmiHeader.biHeight = -renderer.GetHeight();
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            
            SetStretchBltMode(hSmallDC, HALFTONE);
        }
        
        // 缩放截屏
        StretchBlt(hSmallDC, 0, 0, renderer.GetWidth(), renderer.GetHeight(), 
                   hScreenDC, 0, 0, screenW, screenH, SRCCOPY);
                   
        // 获取像素
        GetDIBits(hSmallDC, hSmallBmp, 0, renderer.GetHeight(), 
                  pixels.data(), &bmi, DIB_RGB_COLORS);

        // 2. 处理 (Render to Internal Buffer)
        renderer.Render(pixels);

        // 3. 绘制 (Present Internal Buffer to Window)
        renderer.Draw(hWinDC);

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
