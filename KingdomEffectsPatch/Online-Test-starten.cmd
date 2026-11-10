@echo off
setlocal
pushd "%~dp0" || exit /b 1
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0TestSession.ps1" -Online
set "PATCH_RESULT=%ERRORLEVEL%"
if not "%PATCH_RESULT%"=="0" pause
popd
exit /b %PATCH_RESULT%
