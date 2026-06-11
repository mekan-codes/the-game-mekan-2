@echo off
cd /d "%~dp0"
set "PATH=%~dp0runtime;%PATH%"
"%~dp0crane_game.exe" %*
