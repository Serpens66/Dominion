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
        $process = Start-Process -FilePath $game -WorkingDirectory (Split-Path $game) -WindowStyle Normal -PassThru
        try {
            & (Join-Path $PSScriptRoot 'bin\attach.exe') --installed --apply $process.Id $game $game
            if ($LASTEXITCODE -ne 0) { throw 'Der Patch wurde nicht erfolgreich aktiviert. Bitte Dominion schliessen und den Starter erneut oeffnen.' }
            Write-Host 'Bereit. In Lokal spielen oder Weitergeben die Koenigreich-Auswahl oeffnen und F8 druecken.'
            Write-Host 'Erweiterte Online-Banns stehen in der normalen Sammlung zur Verfuegung.'
            $process.WaitForExit()
        } finally {
            # Keep the session lock while Dominion is running, including on attach failure.
            if (!$process.HasExited) { Write-Host 'Bitte Dominion schliessen. Der Starter wartet auf das Spielende.'; $process.WaitForExit() }
        }
    } finally { if ($lock) { $lock.Dispose() } }
}

try { Invoke-ModRelease } catch { Write-Host $_.Exception.Message; exit 1 }
