@echo off
setlocal
cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
    echo g++ was not found.
    echo.
    echo Install MinGW-w64 and add its bin folder to PATH.
    echo Then open a new terminal and run this file again.
    pause
    exit /b 1
)

echo Building the Hangman server...
g++ backend\main.cpp -std=c++17 -Wall -Wextra -o hangman.exe -lws2_32

if errorlevel 1 (
    echo.
    echo Build failed. Read the error message above.
    pause
    exit /b 1
)

echo.
echo Build complete: hangman.exe
echo Run run.bat to start the game.
pause
