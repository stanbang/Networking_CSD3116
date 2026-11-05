@echo off
rem Start DDS Asteroids in the default Ubuntu WSL distribution. setlocal
title DDS Asteroids
where wsl.exe >nul 2>nul
if errorlevel 1 (
  echo Windows Subsystem for Linux is required. Open this game in Ubuntu 24.04 WSL.
  pause
  exit /b 1
)
echo Starting DDS Asteroids in WSL. The first compilation can take about a minute.
echo Keep this window open while playing. Two game windows will open after the build.
echo.
wsl.exe --cd "%~dp0." -- bash ./run.sh %*
set "GAME_EXIT=%ERRORLEVEL%"
if not "%GAME_EXIT%"=="0" echo Game failed with exit code %GAME_EXIT%. Please copy the error above.
if not "%DDS_ASTEROIDS_NO_PAUSE%"=="1" pause
exit /b %GAME_EXIT%
