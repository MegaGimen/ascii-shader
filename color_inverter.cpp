#include "color_inverter.h"
#include <windows.h>
#include <iostream>

// 手动定义 MAGCOLOREFFECT 结构体，避免依赖 magnification.h
typedef struct tagMAGCOLOREFFECT {
  float transform[5][5];
} MAGCOLOREFFECT, *PMAGCOLOREFFECT;

// 定义函数指针类型
typedef BOOL (WINAPI *MagInitializeFunc)();
typedef BOOL (WINAPI *MagUninitializeFunc)();
typedef BOOL (WINAPI *MagSetFullscreenColorEffectFunc)(PMAGCOLOREFFECT);

// 全局变量存储函数指针和模块句柄
static HMODULE hMagLib = NULL;
static MagInitializeFunc pMagInitialize = NULL;
static MagUninitializeFunc pMagUninitialize = NULL;
static MagSetFullscreenColorEffectFunc pMagSetFullscreenColorEffect = NULL;

namespace ColorInverter {

    bool Initialize() {
        if (hMagLib != NULL) return true; // 已经初始化

        hMagLib = LoadLibraryA("Magnification.dll");
        if (hMagLib == NULL) {
            std::cerr << "Failed to load Magnification.dll" << std::endl;
            return false;
        }

        pMagInitialize = (MagInitializeFunc)GetProcAddress(hMagLib, "MagInitialize");
        pMagUninitialize = (MagUninitializeFunc)GetProcAddress(hMagLib, "MagUninitialize");
        pMagSetFullscreenColorEffect = (MagSetFullscreenColorEffectFunc)GetProcAddress(hMagLib, "MagSetFullscreenColorEffect");

        if (!pMagInitialize || !pMagUninitialize || !pMagSetFullscreenColorEffect) {
            std::cerr << "Failed to get function addresses from Magnification.dll" << std::endl;
            FreeLibrary(hMagLib);
            hMagLib = NULL;
            return false;
        }

        return pMagInitialize();
    }

    void Uninitialize() {
        if (hMagLib) {
            if (pMagUninitialize) {
                pMagUninitialize();
            }
            FreeLibrary(hMagLib);
            hMagLib = NULL;
            pMagInitialize = NULL;
            pMagUninitialize = NULL;
            pMagSetFullscreenColorEffect = NULL;
        }
    }

    bool SetInvert(bool enable) {
        if (!pMagSetFullscreenColorEffect) return false;

        // 颜色变换矩阵 (5x5)
        // 正常矩阵 (Identity)
        MAGCOLOREFFECT identity = {
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
        };

        // 反色矩阵
        // R' = -1*R + 1
        // G' = -1*G + 1
        // B' = -1*B + 1
        MAGCOLOREFFECT invert = {
            -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
             0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
             0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
             0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f
        };

        return pMagSetFullscreenColorEffect(enable ? &invert : &identity);
    }
}
