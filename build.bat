@echo off
g++ main.cpp ascii_renderer.cpp -o screen_ascii.exe -lgdi32
if %errorlevel% neq 0 (
    echo Compilation failed!
    pause
) else (
    echo Compilation successful! Run screen_ascii.exe to test.
)
