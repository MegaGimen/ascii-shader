#include <iostream>
#include <windows.h>
#include "color_inverter.h"

int main() {
    std::cout << "Screen Color Inverter Started." << std::endl;
    std::cout << "Initializing Magnification API..." << std::endl;

    if (!ColorInverter::Initialize()) {
        std::cerr << "Error: Failed to initialize Magnification API." << std::endl;
        std::cerr << "Please try running as Administrator." << std::endl;
        return 1;
    }

    std::cout << "Inverting screen colors..." << std::endl;
    if (!ColorInverter::SetInvert(true)) {
        std::cerr << "Error: Failed to set screen color effect." << std::endl;
        ColorInverter::Uninitialize();
        return 1;
    }

    std::cout << "Screen colors inverted." << std::endl;
    std::cout << "Press 'q' or 'Q' to restore colors and exit." << std::endl;

    // 主循环
    bool running = true;
    while (running) {
        // 检测 'Q' 键
        // 0x8000 表示当前键被按下
        if (GetAsyncKeyState('Q') & 0x8000) {
            running = false;
        }
        
        Sleep(50); // 避免占用过多 CPU
    }

    std::cout << "Restoring colors..." << std::endl;
    ColorInverter::SetInvert(false);
    ColorInverter::Uninitialize();
    std::cout << "Exiting." << std::endl;

    return 0;
}
