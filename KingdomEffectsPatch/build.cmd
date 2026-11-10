@echo off
setlocal
set "PATCH_VS="
for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "PATCH_VS=%%I"
if not defined PATCH_VS (
    echo Visual Studio C++ x86 Build Tools not found.
    exit /b 1
)
call "%PATCH_VS%\VC\Auxiliary\Build\vcvars32.bat" || exit /b 1
set "PATCH_OUT=%~dp0out"
if /i "%~1"=="staging" set "PATCH_OUT=%~dp0out\staging"
if not exist "%PATCH_OUT%" mkdir "%PATCH_OUT%"
pushd "%PATCH_OUT%" || exit /b 1
if /i "%~1"=="background" goto background
"D:\CDesktopLink\Portable\Python\WinPy64\python\python.exe" -B "%~dp0generate_effect_catalog.py" "%PATCH_OUT%\effect_catalog.generated.hpp" "E:\ProgrammeE\Steam\steamapps\common\Dominion\Dominion.xpack" || goto fail
rc /nologo /c65001 /fo effect_editor.res "%~dp0effect_editor.rc" || goto fail
cl /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT /LD /I "%PATCH_OUT%" "%~dp0patch.cpp" "%~dp0effect_editor.cpp" "%~dp0card_picker.cpp" effect_editor.res /link /OUT:KingdomEffectsPatch.dll user32.lib comctl32.lib gdiplus.lib || goto fail
cl /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0attach.cpp" /link /OUT:attach.exe bcrypt.lib || goto fail
cl /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0effects_test.cpp" /link /OUT:effects_test.exe || goto fail
effects_test.exe || goto fail
goto done
:background
cl /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0background_effect_test.cpp" /link /OUT:background_effect_test.exe bcrypt.lib user32.lib || goto fail
:done
popd
exit /b 0
:fail
popd
exit /b 1
