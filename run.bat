@echo off
setlocal
cd /d "%~dp0"

if not exist "hangman.exe" (
    echo hangman.exe was not found.
    echo Run build.bat first.
    pause
    exit /b 1
)

echo Starting the Hangman server...
echo Open http://localhost:8081 in your browser.
echo Press Ctrl+C to stop the server.
echo.

hangman.exe %*

echo.
echo The server has stopped.
pause
