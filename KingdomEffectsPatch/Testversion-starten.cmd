@echo off
setlocal
if not exist "%~dp0out\Test-v0.1.3-zoom-fix\KingdomEffectsPatch-v0.1.3\Dominion-mit-Mod-starten.cmd" (
    echo Das lokale Testpaket v0.1.3 fehlt.
    pause
    exit /b 1
)
call "%~dp0out\Test-v0.1.3-zoom-fix\KingdomEffectsPatch-v0.1.3\Dominion-mit-Mod-starten.cmd"
exit /b %ERRORLEVEL%
