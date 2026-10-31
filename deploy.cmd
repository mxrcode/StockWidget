@echo off
rem Double-click launcher for deploy.ps1 (bypasses the PowerShell execution policy for this script only)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0deploy.ps1" %*
if errorlevel 1 (
    echo.
    echo Deploy FAILED
)
pause
