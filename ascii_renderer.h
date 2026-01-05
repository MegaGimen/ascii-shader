#ifndef ASCII_RENDERER_H
#define ASCII_RENDERER_H

#include <windows.h>
#include <string>
#include <vector>

class AsciiRenderer {
public:
    AsciiRenderer();
    ~AsciiRenderer();

    // 初始化
    // width: ASCII 字符宽度 (列数)
    bool Initialize(int width);

    // 处理并绘制
    // hSrcDC: 源 DC (包含屏幕截图)
    // hDestDC: 目标 DC (绘制窗口)
    // screenW, screenH: 屏幕宽高
    void ProcessAndDraw(HDC hSrcDC, HDC hDestDC, int screenW, int screenH);

private:
    int m_width;     // ASCII 列数
    int m_height;    // ASCII 行数 (根据屏幕比例计算)
    
    HDC m_hSmallDC;      // 用于缩放的小图 DC
    HBITMAP m_hSmallBmp; // 用于缩放的小位图
    HBITMAP m_hOldSmallBmp;
    
    HFONT m_hFont;       // 用于绘制的字体
    int m_fontWidth;     // 字体宽
    int m_fontHeight;    // 字体高

    std::string m_asciiTable; // 字符映射表
    
    // 预分配像素缓冲区
    std::vector<RGBQUAD> m_pixels;
    BITMAPINFO m_bmi;
};

#endif // ASCII_RENDERER_H
