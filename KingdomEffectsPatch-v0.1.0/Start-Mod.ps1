param([string]$GameDirectory = '', [switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ReleaseSupport.ps1')

function Invoke-ModRelease {
    Assert-ModGameClosed
    Assert-ModPackage $PSScriptRoot
    $state = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'DominionKingdomEffectsPatch'
    New-Item -ItemType Directory -Path $state -Force | Out-Null
    Assert-NormalDirectory $state
    $lock = $null
    try {
        try { $lock = [IO.File]::Open((Join-Path $state 'session.lock'), 'OpenOrCreate', 'ReadWrite', 'None') }
        catch { throw 'Die Sitzung ist bereits geoeffnet oder ihre Sperrdatei nicht zugreifbar. Bitte andere Starter schliessen und die Zugriffsrechte des Mod-Datenordners pruefen.' }
        Assert-ModGameClosed
        $source = Resolve-ModInstallation $state $GameDirectory
        $profileParent = Join-Path ([Environment]::GetFolderPath('ApplicationData')) 'TempleGates'
        # Never take over an unfinished legacy test-profile swap.
        $legacyJournal = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'DominionModTests\3315\session.state'
        if ((Test-Path -LiteralPath (Join-Path $profileParent 'Dominion.KingdomEffectsPatch.original')) -or
            (Test-Path -LiteralPath $legacyJournal)) {
            throw 'Eine alte Testsitzung hat das Originalprofil noch ausgelagert. Bitte zuerst den bisherigen Test-Starter oeffnen und die Wiederherstellung abschliessen.'
        }
        if ($CheckOnly) { Write-Host "Paket und Spielbuild geprueft: $source"; return }
        Write-Host 'Dominion Kingdom Effects Patch v0.1.0 | Spielbuild 3315'
        $game = Join-Path $source 'Dominion.exe'
        Assert-ModGameClosed
        $backup = Join-Path $state 'profile-backups'
        Backup-ModProfile (Join-Path $profileParent 'Dominion\profiles.xml') $backup
        Write-Host "Normales Spielprofil. Bis zu drei Sicherungen unter: $backup"
        $start = New-Object System.Diagnostics.ProcessStartInfo
        $start.FileName = $game
        $start.WorkingDirectory = Split-Path $game
        $start.UseShellExecute = $false
        # SteamAPI needs an app ID when launching the installed EXE directly.
        # Only the child environment changes; no steam_appid.txt or user setting.
        $start.EnvironmentVariables['SteamAppId'] = '1131620'
        $start.EnvironmentVariables['SteamGameId'] = '1131620'
        $process = [Diagnostics.Process]::Start($start)
        Write-Host "Spielprozess gestartet: PID $($process.Id)"
        try {
            $savedPreference = $ErrorActionPreference
            try {
                $ErrorActionPreference = 'Continue'
                & (Join-Path $PSScriptRoot 'bin\attach.exe') --installed --apply $process.Id $game $game 2>&1 |
                    ForEach-Object { Write-Host $_ }
                $attachResult = $LASTEXITCODE
            } finally { $ErrorActionPreference = $savedPreference }
            Write-Host "Injector-Ergebnis: $attachResult"
            if ($attachResult -ne 0) { throw 'Der Patch wurde nicht erfolgreich aktiviert. Bitte Dominion schliessen und den Starter erneut oeffnen.' }
            Write-Host 'Bereit. In Lokal spielen, Weitergeben oder Online/Privates Spiel die Koenigreich-Auswahl oeffnen und F8 druecken.'
            Write-Host 'Erweiterte Online-Banns stehen in der normalen Sammlung zur Verfuegung.'
            $process.WaitForExit()
        } finally {
            # Keep the session lock while Dominion is running, including on attach failure.
            if (!$process.HasExited) { Write-Host 'Bitte Dominion schliessen. Der Starter wartet auf das Spielende.'; $process.WaitForExit() }
            $patchLog = Join-Path ([IO.Path]::GetTempPath()) "DominionKingdomEffects-$($process.Id).log"
            if (Test-Path -LiteralPath $patchLog) {
                Copy-Item -LiteralPath $patchLog -Destination (Join-Path $state 'letzter-patch.log') -Force
                Write-Host "Patch-Protokoll: $(Join-Path $state 'letzter-patch.log')"
            }
        }
    } finally { if ($lock) { $lock.Dispose() } }
}

$transcribing = $false
try {
    $logRoot = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'DominionKingdomEffectsPatch'
    New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
    Assert-NormalDirectory $logRoot
    $log = Join-Path $logRoot 'letzter-start.log'
    if (Test-Path -LiteralPath $log) { Copy-Item -LiteralPath $log -Destination (Join-Path $logRoot 'vorheriger-start.log') -Force }
    Start-Transcript -LiteralPath $log -Force | Out-Null
    $transcribing = $true
    Write-Host "Startprotokoll: $log"
    Invoke-ModRelease
} catch {
    Write-Host ('Start fehlgeschlagen: ' + $_.Exception.Message)
    exit 1
} finally {
    if ($transcribing) { Stop-Transcript | Out-Null }
}
