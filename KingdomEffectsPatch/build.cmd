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
if /i "%~1"=="probes" set "PATCH_OUT=%~dp0out\staging"
if not exist "%PATCH_OUT%" mkdir "%PATCH_OUT%"
if not defined DOMINION_PYTHON set "DOMINION_PYTHON=D:\CDesktopLink\Portable\Python\WinPy64\python\python.exe"
if not exist "%DOMINION_PYTHON%" set "DOMINION_PYTHON=python"
if not defined DOMINION_GAME_DIRECTORY set "DOMINION_GAME_DIRECTORY=E:\ProgrammeE\Steam\steamapps\common\Dominion"
pushd "%PATCH_OUT%" || exit /b 1
if /i "%~1"=="background" goto background
if /i "%~1"=="probes" goto probes
"%DOMINION_PYTHON%" -B "%~dp0generate_effect_catalog.py" "%PATCH_OUT%\effect_catalog.generated.hpp" "%DOMINION_GAME_DIRECTORY%\Dominion.xpack" || goto fail
rc /nologo /c65001 /fo effect_editor.res "%~dp0effect_editor.rc" || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT /LD "%~dp0patch.cpp" "%~dp0effect_editor.cpp" "%~dp0card_picker.cpp" "%~dp0dialog_skin.cpp" "%~dp0game_resources.cpp" effect_editor.res /link /OUT:KingdomEffectsPatch.dll user32.lib gdi32.lib comctl32.lib gdiplus.lib || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0attach.cpp" /link /OUT:attach.exe bcrypt.lib || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0effects_test.cpp" /link /OUT:effects_test.exe || goto fail
effects_test.exe || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0launcher_test.cpp" /link /OUT:launcher_test.exe bcrypt.lib || goto fail
launcher_test.exe || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0clipboard_test.cpp" /link /OUT:clipboard_test.exe user32.lib gdi32.lib comctl32.lib || goto fail
clipboard_test.exe || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0resources_test.cpp" /link /OUT:resources_test.exe gdiplus.lib user32.lib gdi32.lib || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0release_game_test.cpp" /link /OUT:release_game_test.exe bcrypt.lib user32.lib || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0editor_release_test.cpp" "%~dp0dialog_skin.cpp" "%~dp0game_resources.cpp" effect_editor.res /link /OUT:editor_release_test.exe user32.lib gdi32.lib comctl32.lib gdiplus.lib || goto fail
:probes
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT /LD "%~dp0roundtrip_probe.cpp" "%~dp0effect_editor.cpp" "%~dp0card_picker.cpp" "%~dp0dialog_skin.cpp" "%~dp0game_resources.cpp" effect_editor.res /link /OUT:roundtrip_probe.dll user32.lib gdi32.lib comctl32.lib gdiplus.lib || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0roundtrip_test.cpp" /link /OUT:roundtrip_test.exe bcrypt.lib user32.lib || goto fail
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0startup_effect_test.cpp" /link /OUT:startup_effect_test.exe bcrypt.lib user32.lib || goto fail
goto done
:background
cl /I "%PATCH_OUT%" /nologo /W4 /O2 /EHsc /std:c++17 /utf-8 /MT "%~dp0background_effect_test.cpp" /link /OUT:background_effect_test.exe bcrypt.lib user32.lib || goto fail
:done
popd
exit /b 0
:fail
popd
exit /b 1
