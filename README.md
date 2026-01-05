# ASCII Shader for Windows

一个高性能的 Windows 屏幕滤镜，能够将你的整个显示器画面实时转换为 ASCII 字符画。

该项目使用 C++ 编写，通过纯内存操作（Software Rendering）实现了极高的性能，能够以极低的 CPU 占用率实时渲染高清 ASCII 画面。

## ✨ 特性

- **实时转换**：捕获屏幕内容并实时转换为彩色 ASCII 字符。
- **高性能**：
  - 使用纯内存缓冲区操作，避免昂贵的 GDI `TextOut` 调用。
  - 预计算字体纹理 (Font Atlas) 加速渲染。
  - 针对 CPU 缓存优化的渲染循环。
- **无感化体验**：
  - 窗口完全鼠标穿透 (Click-through)。
  - 不抢占焦点 (No Activate)。
  - 任务栏隐藏。
  - 就像一个贴在屏幕上的透明保护膜。
- **防递归设计**：使用 `SetWindowDisplayAffinity` 防止程序捕获到自身产生的画面（避免“无限镜”效应）。
- **自定义分辨率**：支持任意宽度的 ASCII 分辨率，自动保持屏幕比例。
- **DPI 感知**：完美支持高分屏 (HiDPI)，点对点渲染。

## 🛠️ 编译

项目仅依赖 Windows API (`gdi32`)，无需任何第三方库。

### 依赖
- MinGW (g++) 或其他支持 C++11 的 Windows 编译器。

### 编译命令
直接运行根目录下的 `build.bat`，或者在终端执行：

```bash
g++ main.cpp ascii_renderer.cpp -o screen_ascii.exe -lgdi32
```

## 🚀 使用方法

### 1. 基础运行
直接双击 `screen_ascii.exe` 或在终端运行：

```bash
.\screen_ascii.exe
```

默认使用 150 字符宽度。

### 2. 指定分辨率
你可以通过命令行参数指定 ASCII 网格的**宽度**（列数）。程序会自动计算高度以保持屏幕比例。

**低分辨率 (复古艺术风):**
```bash
.\screen_ascii.exe 150
```

**中分辨率 (平衡):**
```bash
.\screen_ascii.exe 300
```

**高分辨率 (清晰可读):**
```bash
.\screen_ascii.exe 600
```

**原生分辨率 (1:1 像素映射):**
如果你的屏幕是 1920x1080，你可以设置为 1920。注意这会生成极小的字符。
```bash
.\screen_ascii.exe 1920
```

### 3. 退出
- 按下键盘上的 **`Q`** 键即可退出程序。

## 🧩 技术原理

1.  **截屏**：使用 GDI `BitBlt` 捕获当前屏幕帧。
2.  **缩放**：将高清截屏缩放到目标 ASCII 网格大小（例如 1920x1080 -> 150x84）。
3.  **灰度映射**：读取每个像素的亮度，映射到 ASCII 字符集 (` .:-=+*#%@` 等)。
4.  **软件渲染**：
    - 程序启动时生成一张包含所有字符像素数据的 Font Atlas。
    - 在渲染循环中，直接查找 Atlas 并将像素拷贝到最终的屏幕缓冲区 (`std::vector<uint32_t>`)。
    - 这一步完全绕过了 Windows GDI 的绘制指令，速度极快。
5.  **上屏**：使用 `SetDIBitsToDevice` 一次性将合成好的图像推送到屏幕。

## 📝 注意事项

- 程序使用了 `SetWindowDisplayAffinity` 来防止截屏递归。这需要 Windows 10 2004 或更高版本。
- 如果你在 OBS 或直播软件中使用，该窗口可能会变为全黑（这是为了防止直播画面无限递归）。

## 📄 License

MIT License
