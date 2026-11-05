@echo off
rem Reopen player2 only; leave the existing server and other player running.
setlocal
title DDS Asteroids - Reconnect player 2
wsl.exe --cd "%~dp0." -- bash ./tools/rejoin.sh 2 %*
set "GAME_EXIT=%ERRORLEVEL%"
if not "%GAME_EXIT%"=="0" echo Reconnect failed. If the round ended, start a new game with RunGame.cmd.
if not "%DDS_ASTEROIDS_NO_PAUSE%"=="1" pause
exit /b %GAME_EXIT%
