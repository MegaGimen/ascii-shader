#include "ascii_renderer.h"
#include <iostream>
#include <cmath>

AsciiRenderer::AsciiRenderer() 
    : m_width(0), m_height(0), 
      m_hSmallDC(NULL), m_hSmallBmp(NULL), m_hOldSmallBmp(NULL), 
      m_hFont(NULL), m_fontWidth(0), m_fontHeight(0) 
{
    // 初始化字符表 (Detailed)
    m_asciiTable = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
}

AsciiRenderer::~AsciiRenderer() {
    if (m_hSmallDC) {
        SelectObject(m_hSmallDC, m_hOldSmallBmp);
        DeleteDC(m_hSmallDC);
    }
    if (m_hSmallBmp) DeleteObject(m_hSmallBmp);
    if (m_hFont) DeleteObject(m_hFont);
}

bool AsciiRenderer::Initialize(int width) {
    m_width = width;
    
    // 获取屏幕比例来计算高度
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    
    // ASCII 字符通常是瘦长的 (例如 8x16 像素)。
    // 假设字符宽高比约为 1:2。
    // 为了保持图像比例: (W_char * cols) / (H_char * rows) = ScreenW / ScreenH
    // rows = (W_char * cols * ScreenH) / (H_char * ScreenW)
    // 假设 W_char/H_char = 0.5 (Consolas 大概是这个比例)
    // rows = (0.5 * cols * ScreenH) / ScreenW
    
    double charAspectRatio = 0.5; 
    m_height = (int)((charAspectRatio * m_width * screenH) / screenW);
    
    if (m_height < 1) m_height = 1;

    // 创建小图 DC
    HDC hScreenDC = GetDC(NULL);
    m_hSmallDC = CreateCompatibleDC(hScreenDC);
    m_hSmallBmp = CreateCompatibleBitmap(hScreenDC, m_width, m_height);
    m_hOldSmallBmp = (HBITMAP)SelectObject(m_hSmallDC, m_hSmallBmp);
    ReleaseDC(NULL, hScreenDC);

    // 设置拉伸模式为半色调，质量更好
    SetStretchBltMode(m_hSmallDC, HALFTONE);
    SetBrushOrgEx(m_hSmallDC, 0, 0, NULL);

    // 创建字体
    // 计算目标字体大小
    // 目标屏幕宽度 = screenW
    // 字符数 = m_width
    // 单个字符宽度 = screenW / m_width
    m_fontWidth = screenW / m_width;
    // 保持 1:2 比例
    m_fontHeight = m_fontWidth * 2; 

    m_hFont = CreateFontA(
        m_fontHeight,       // Height
        m_fontWidth,        // Width
        0, 0,               // Escapement, Orientation
        FW_NORMAL,          // Weight
        FALSE, FALSE, FALSE,// Italic, Underline, StrikeOut
        ANSI_CHARSET,       // CharSet
        OUT_DEFAULT_PRECIS, // OutPrecision
        CLIP_DEFAULT_PRECIS,// ClipPrecision
        ANTIALIASED_QUALITY,// Quality
        FIXED_PITCH | FF_MODERN, // PitchAndFamily
        "Consolas"          // FaceName
    );

    // 预分配像素缓冲区
    m_pixels.resize(m_width * m_height);
    
    ZeroMemory(&m_bmi, sizeof(BITMAPINFO));
    m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bmi.bmiHeader.biWidth = m_width;
    m_bmi.bmiHeader.biHeight = -m_height; // 负值表示自上而下
    m_bmi.bmiHeader.biPlanes = 1;
    m_bmi.bmiHeader.biBitCount = 32;
    m_bmi.bmiHeader.biCompression = BI_RGB;

    return true;
}

void AsciiRenderer::ProcessAndDraw(HDC hSrcDC, HDC hDestDC, int screenW, int screenH) {
    // 1. 缩放截图到小图
    StretchBlt(m_hSmallDC, 0, 0, m_width, m_height, 
               hSrcDC, 0, 0, screenW, screenH, SRCCOPY);

    // 2. 获取像素数据
    GetDIBits(m_hSmallDC, m_hSmallBmp, 0, m_height, 
              m_pixels.data(), &m_bmi, DIB_RGB_COLORS);

    // 3. 准备绘制
    // 填充黑色背景
    RECT rect = {0, 0, screenW, screenH};
    FillRect(hDestDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    HFONT hOldFont = (HFONT)SelectObject(hDestDC, m_hFont);
    SetBkMode(hDestDC, TRANSPARENT);

    // 4. 遍历像素并绘制字符
    int tableLen = m_asciiTable.length();
    int xPos = 0;
    int yPos = 0;

    // 预计算坐标映射以避免浮点运算
    // 其实既然我们是固定宽度的字体，直接累加即可
    // xPos = col * m_fontWidth
    // yPos = row * m_fontHeight

    // 优化：使用 SetTextColor 和 TextOut 还是比较慢
    // 但在没有 Direct2D 的情况下这是最简单的
    // 优化2：ExtTextOut 通常比 TextOut 快一点点
    // 优化3：缓存颜色，避免重复调用 SetTextColor
    // 优化4：批处理绘制 (Batch Drawing) - 极大幅度减少 syscall

    COLORREF lastColor = CLR_INVALID;
    std::string currentString;
    currentString.reserve(m_width); // 预分配
    int startX = 0;

    for (int y = 0; y < m_height; ++y) {
        xPos = 0;
        startX = 0; // 每行开始重置 startX
        currentString.clear();
        lastColor = CLR_INVALID; // 每行开始重置颜色状态，强制第一次设置颜色

        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            RGBQUAD& p = m_pixels[idx];

            // 计算灰度
            // Y = 0.299R + 0.587G + 0.114B
            // 为了速度使用整数运算: (299*R + 587*G + 114*B) / 1000
            int gray = (299 * p.rgbRed + 587 * p.rgbGreen + 114 * p.rgbBlue) / 1000;
            
            // 映射到字符
            int charIdx = (gray * (tableLen - 1)) / 255;
            char c = m_asciiTable[charIdx];

            // 设置颜色 (仅当颜色变化时)
            COLORREF currentColor = RGB(p.rgbRed, p.rgbGreen, p.rgbBlue);
            /* 逻辑移动到了下方 */
            
            // 绘制字符
            // ETO_OPAQUE: 用背景色填充矩形 (这里不需要，因为我们已经清屏且 SetBkMode 为 TRANSPARENT)
            // TextOutA(hDestDC, xPos, yPos, &c, 1);
            // ExtTextOutA(hDestDC, xPos, yPos, 0, NULL, &c, 1, NULL);
            
            // 批处理优化逻辑：
            // 我们不立即绘制，而是检测“当前字符颜色是否与上一个相同”
            // 如果相同，我们只增加 buffer 里的字符
            // 如果不同，或者换行了，我们就把 buffer 里的字符串一次性画出来
            
            if (currentColor == lastColor && currentString.length() < 256) {
                // 颜色相同，追加字符
                currentString += c;
            } else {
                // 颜色不同，先画出之前的
                if (!currentString.empty()) {
                    // 设置之前的颜色
                    SetTextColor(hDestDC, lastColor);
                    // 一次性绘制一串字符
                    ExtTextOutA(hDestDC, startX, yPos, 0, NULL, currentString.c_str(), currentString.length(), NULL);
                }
                
                // 重置状态为当前新颜色/新字符
                lastColor = currentColor;
                currentString = c;
                startX = xPos;
            }
            
            xPos += m_fontWidth;
        }
        
        // 行末：必须把缓冲区剩下的画出来
        if (!currentString.empty()) {
            SetTextColor(hDestDC, lastColor);
            ExtTextOutA(hDestDC, startX, yPos, 0, NULL, currentString.c_str(), currentString.length(), NULL);
            currentString.clear();
        }
        
        yPos += m_fontHeight;
    }

    SelectObject(hDestDC, hOldFont);
}
