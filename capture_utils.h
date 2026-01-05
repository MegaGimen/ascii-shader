#ifndef CAPTURE_UTILS_H
#define CAPTURE_UTILS_H

#include <windows.h>
#include <gdiplus.h>
#include <string>

// Link with -lgdiplus
// Initialize GDI+ before using these functions

namespace CaptureUtils {
    // 初始化 GDI+
    bool Initialize(ULONG_PTR& gdiplusToken);
    
    // 清理 GDI+
    void Uninitialize(ULONG_PTR gdiplusToken);

    // 获取屏幕截图并保存为 PNG
    // filename: 保存路径
    // width, height: 屏幕宽高
    bool CaptureScreenToFile(const std::wstring& filename, int& width, int& height);

    // 加载图片并绘制到指定 DC
    bool LoadAndDraw(HDC hdc, const std::wstring& filename, int x, int y, int width, int height);

    // 获取用于 PNG 编码的 CLSID
    bool GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
}

#endif // CAPTURE_UTILS_H
