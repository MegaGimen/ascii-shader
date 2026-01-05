#include "ascii_renderer.h"
#include <iostream>
#include <cmath>
#include <cstdint>
#include <algorithm>

AsciiRenderer::AsciiRenderer() 
    : m_width(0), m_height(0), m_screenWidth(0), m_screenHeight(0),
      m_fontWidth(0), m_fontHeight(0), m_hFont(NULL)
{
    // 初始化字符表
    m_asciiTable = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
}

AsciiRenderer::~AsciiRenderer() {
    if (m_hFont) DeleteObject(m_hFont);
}

bool AsciiRenderer::Initialize(int targetWidth) {
    // 获取屏幕分辨率
    m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // 智能计算：优先保证铺满屏幕，而不是严格遵守 targetWidth
    // 1. 根据用户想要的分辨率，计算理想的字体像素宽度 (浮点数)
    float idealFontWidth = (float)m_screenWidth / targetWidth;
    
    // 2. 四舍五入到最近的整数像素 (至少为 1)
    m_fontWidth = (int)std::round(idealFontWidth);
    if (m_fontWidth < 1) m_fontWidth = 1;
    
    // 3. 反推实际能铺满屏幕的列数
    // 使用 ceil 确保即使有余数也覆盖，或者 floor 留极小黑边
    // 这里使用除法，并在渲染时处理边界，保证最大化利用屏幕
    m_width = m_screenWidth / m_fontWidth;
    // 如果余数超过一半字体宽，可以考虑加一列（会被裁剪）
    // 但为了简单，先这样。余数最大为 fontWidth - 1 像素。
    
    // 4. 假设字符高宽比 2:1 (Consolas)
    m_fontHeight = m_fontWidth * 2;
    
    // 5. 计算行数
    m_height = m_screenHeight / m_fontHeight;
    
    // 打印调试信息 (在控制台可见)
    std::cout << "Target Width: " << targetWidth << std::endl;
    std::cout << "Adjusted Resolution: " << m_width << "x" << m_height << std::endl;
    std::cout << "Font Size: " << m_fontWidth << "x" << m_fontHeight << " px" << std::endl;

    if (m_width < 1) m_width = 1;
    if (m_height < 1) m_height = 1;

    // 创建字体用于生成 Atlas
    if (m_hFont) DeleteObject(m_hFont);
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

    // 初始化屏幕缓冲区
    // 大小必须是屏幕的完整像素数
    m_screenBuffer.resize(m_screenWidth * m_screenHeight);
    
    // 设置 BITMAPINFO
    ZeroMemory(&m_bmi, sizeof(BITMAPINFO));
    m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bmi.bmiHeader.biWidth = m_screenWidth;
    m_bmi.bmiHeader.biHeight = -m_screenHeight; // 自上而下
    m_bmi.bmiHeader.biPlanes = 1;
    m_bmi.bmiHeader.biBitCount = 32; // 32-bit RGB
    m_bmi.bmiHeader.biCompression = BI_RGB;

    // 生成字体纹理 Atlas
    PrecomputeFontAtlas();

    return true;
}

void AsciiRenderer::PrecomputeFontAtlas() {
    // 创建一个临时 DC 和 Bitmap 来绘制每个字符
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, m_fontWidth, m_fontHeight);
    HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);
    HGDIOBJ hOldFont = SelectObject(hMemDC, m_hFont);
    
    // 256 个字符，每个字符占 m_fontWidth * m_fontHeight 字节
    m_fontAtlas.resize(256 * m_fontWidth * m_fontHeight);
    std::fill(m_fontAtlas.begin(), m_fontAtlas.end(), 0);

    // 临时缓冲区读取 Bitmap 像素
    std::vector<uint32_t> tempPixels(m_fontWidth * m_fontHeight);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_fontWidth;
    bmi.bmiHeader.biHeight = -m_fontHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    RECT rect = {0, 0, m_fontWidth, m_fontHeight};

    for (int i = 0; i < 256; ++i) {
        // 清黑背景
        FillRect(hMemDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(255, 255, 255)); // 白色文字

        char c = (char)i;
        TextOutA(hMemDC, 0, 0, &c, 1);

        // 读取像素
        GetDIBits(hMemDC, hBitmap, 0, m_fontHeight, tempPixels.data(), &bmi, DIB_RGB_COLORS);

        // 存入 Atlas
        // 任何非黑色像素视为点亮
        int atlasOffset = i * m_fontWidth * m_fontHeight;
        for (int p = 0; p < m_fontWidth * m_fontHeight; ++p) {
            // 简单阈值处理：如果是白色(或非黑)，设为1
            // tempPixels 是 0x00RRGGBB
            if ((tempPixels[p] & 0x00FFFFFF) != 0) {
                m_fontAtlas[atlasOffset + p] = 1;
            } else {
                m_fontAtlas[atlasOffset + p] = 0;
            }
        }
    }

    SelectObject(hMemDC, hOldFont);
    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);
}

void AsciiRenderer::Render(const std::vector<RGBQUAD>& pixels) {
    // pixels 是输入的小图 (m_width * m_height)
    // m_screenBuffer 是输出的大图 (m_screenWidth * m_screenHeight)
    
    // 清空屏幕缓冲区 (黑色背景)
    std::fill(m_screenBuffer.begin(), m_screenBuffer.end(), 0);

    int tableLen = m_asciiTable.length();
    
    // 并行优化提示：这里可以用 OpenMP，但为了简单先单线程
    // 遍历每一个 ASCII 格子
    for (int r = 0; r < m_height; ++r) {
        for (int c = 0; c < m_width; ++c) {
            // 1. 获取字符和颜色
            int pixelIdx = r * m_width + c;
            if (pixelIdx >= pixels.size()) continue;
            
            const RGBQUAD& p = pixels[pixelIdx];
            
            // 计算灰度
            int gray = (299 * p.rgbRed + 587 * p.rgbGreen + 114 * p.rgbBlue) / 1000;
            int charIdx = (gray * (tableLen - 1)) / 255;
            unsigned char asciiChar = (unsigned char)m_asciiTable[charIdx];
            
            uint32_t color = (p.rgbRed << 16) | (p.rgbGreen << 8) | p.rgbBlue;

            // 2. 绘制到屏幕缓冲区
            // 目标屏幕位置
            int startScreenX = c * m_fontWidth;
            int startScreenY = r * m_fontHeight;
            
            // Atlas 偏移
            int atlasOffset = asciiChar * m_fontWidth * m_fontHeight;
            
            for (int fy = 0; fy < m_fontHeight; ++fy) {
                int screenY = startScreenY + fy;
                if (screenY >= m_screenHeight) break;
                
                int screenRowOffset = screenY * m_screenWidth;
                int atlasRowOffset = atlasOffset + fy * m_fontWidth;

                for (int fx = 0; fx < m_fontWidth; ++fx) {
                    int screenX = startScreenX + fx;
                    if (screenX >= m_screenWidth) break;

                    if (m_fontAtlas[atlasRowOffset + fx]) {
                        m_screenBuffer[screenRowOffset + screenX] = color;
                    }
                }
            }
        }
    }
}

void AsciiRenderer::Draw(HDC hDestDC) {
    SetDIBitsToDevice(
        hDestDC,
        0, 0, 
        m_screenWidth, m_screenHeight,
        0, 0,
        0, m_screenHeight,
        m_screenBuffer.data(),
        &m_bmi,
        DIB_RGB_COLORS
    );
}
