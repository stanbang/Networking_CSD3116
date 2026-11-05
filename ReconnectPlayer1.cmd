@echo off
rem @file ReconnectPlayer1.cmd
rem @brief Reopen the Player 1 client in the current local session.
rem Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
rem Project AI-use disclosure: docs/AI_USAGE.md.
rem Reopen player1 only; leave the existing server and other player running.
setlocal
title DDS Asteroids - Reconnect player 1
wsl.exe --cd "%~dp0." -- bash ./tools/rejoin.sh 1 %*
set "GAME_EXIT=%ERRORLEVEL%"
if not "%GAME_EXIT%"=="0" echo Reconnect failed. If the round ended, start a new game with RunGame.cmd.
if not "%DDS_ASTEROIDS_NO_PAUSE%"=="1" pause
exit /b %GAME_EXIT%
