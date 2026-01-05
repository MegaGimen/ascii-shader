#ifndef COLOR_INVERTER_H
#define COLOR_INVERTER_H

namespace ColorInverter {
    // 初始化放大镜API
    bool Initialize();
    
    // 清理资源
    void Uninitialize();
    
    // 设置是否反色
    // enable: true 开启反色, false 关闭反色
    bool SetInvert(bool enable);
}

#endif // COLOR_INVERTER_H
