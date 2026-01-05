#include "capture_utils.h"
#include <iostream>

namespace CaptureUtils {

    bool Initialize(ULONG_PTR& gdiplusToken) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        return Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) == Gdiplus::Ok;
    }

    void Uninitialize(ULONG_PTR gdiplusToken) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
    }

    bool GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
        UINT num = 0;          // number of image encoders
        UINT size = 0;         // size of the image encoder array in bytes

        Gdiplus::GetImageEncodersSize(&num, &size);
        if (size == 0) return false;

        Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
        if (pImageCodecInfo == NULL) return false;

        Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

        for (UINT j = 0; j < num; ++j) {
            if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
                *pClsid = pImageCodecInfo[j].Clsid;
                free(pImageCodecInfo);
                return true;
            }
        }

        free(pImageCodecInfo);
        return false;
    }

    bool CaptureScreenToFile(const std::wstring& filename, int& width, int& height) {
        // 获取桌面 DC
        HDC hScreenDC = GetDC(NULL);
        HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);

        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

        // 复制屏幕内容
        // 使用 SRCCOPY
        BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

        // 创建 GDI+ Bitmap
        Gdiplus::Bitmap bitmap(hBitmap, NULL);

        // 保存为 PNG
        CLSID pngClsid;
        if (GetEncoderClsid(L"image/png", &pngClsid)) {
            bitmap.Save(filename.c_str(), &pngClsid, NULL);
        } else {
            std::cerr << "Failed to get PNG encoder CLSID" << std::endl;
        }

        // 清理 GDI 资源
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);

        return true;
    }

    bool LoadAndDraw(HDC hdc, const std::wstring& filename, int x, int y, int width, int height) {
        Gdiplus::Image image(filename.c_str());
        if (image.GetLastStatus() != Gdiplus::Ok) {
            return false;
        }

        Gdiplus::Graphics graphics(hdc);
        graphics.DrawImage(&image, x, y, width, height);
        return true;
    }
}
