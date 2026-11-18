@echo off
setlocal
echo Unveroeffentlichter Import-Teststand - keine neue GitHub-Version.
if not exist "%~dp0out\Import-Test\KingdomEffectsPatch-v0.1.1\Dominion-mit-Mod-starten.cmd" (
    echo Testpaket fehlt. Bitte zuerst das lokale Import-Testpaket vorbereiten.
    pause
    exit /b 1
)
call "%~dp0out\Import-Test\KingdomEffectsPatch-v0.1.1\Dominion-mit-Mod-starten.cmd"
exit /b %ERRORLEVEL%
