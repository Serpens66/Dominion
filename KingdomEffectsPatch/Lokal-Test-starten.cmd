@echo off
setlocal
pushd "%~dp0" || exit /b 1
set "PATCH_MODE=-Editor"
if /i "%~1"=="/menue" set "PATCH_MODE="
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0TestSession.ps1" %PATCH_MODE%
set "PATCH_RESULT=%ERRORLEVEL%"
if not "%PATCH_RESULT%"=="0" pause
popd
exit /b %PATCH_RESULT%
