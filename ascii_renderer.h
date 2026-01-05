#ifndef ASCII_RENDERER_H
#define ASCII_RENDERER_H

#include <windows.h>
#include <vector>
#include <string>
#include <cstdint>

class AsciiRenderer {
public:
    AsciiRenderer();
    ~AsciiRenderer();

    // 初始化渲染器，指定一行有多少个字符
    bool Initialize(int width);
    
    // 输入屏幕截图数据，进行处理
    void Render(const std::vector<RGBQUAD>& pixels);
    
    // 将结果绘制到目标设备上下文
    void Draw(HDC hDestDC);

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    void PrecomputeFontAtlas();

private:
    int m_width;        // ASCII 字符列数
    int m_height;       // ASCII 字符行数
    int m_screenWidth;
    int m_screenHeight;
    int m_fontWidth;
    int m_fontHeight;
    
    std::string m_asciiTable;
    
    // GDI 资源 (仅用于生成 Atlas)
    HFONT m_hFont;
    
    // 软件渲染缓冲区
    std::vector<uint32_t> m_screenBuffer; // 最终屏幕像素 (0x00RRGGBB)
    std::vector<uint8_t> m_fontAtlas;     // 字体纹理 (256 * fontH * fontW)
    BITMAPINFO m_bmi;                     // 用于 SetDIBitsToDevice
};

#endif // ASCII_RENDERER_H
