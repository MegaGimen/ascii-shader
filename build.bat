@echo off
g++ main.cpp color_inverter.cpp -o screen_inverter.exe
if %errorlevel% neq 0 (
    echo Compilation failed!
    pause
) else (
    echo Compilation successful! Run screen_inverter.exe to test.
)
