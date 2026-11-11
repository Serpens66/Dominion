param([switch]$Online, [switch]$Editor)

$ErrorActionPreference = 'Stop'

function Assert-GameClosed {
    if (Get-Process Dominion -ErrorAction SilentlyContinue) {
        throw 'Bitte zuerst alle Dominion-Fenster schliessen.'
    }
}

function Get-ProfilePaths {
    param([string]$Parent, [string]$Journal)
    $root = (Get-Item -LiteralPath $Parent).FullName
    $paths = @{
        Parent = $root
        Active = Join-Path $root 'Dominion'
        Original = Join-Path $root 'Dominion.KingdomEffectsPatch.original'
        Test = Join-Path $root 'Dominion.KingdomEffectsPatch.test'
        Journal = $Journal
    }
    foreach ($key in 'Active', 'Original', 'Test') {
        $path = $paths[$key]
        if (![string]::Equals([IO.Path]::GetDirectoryName($path), $root,
                             [StringComparison]::OrdinalIgnoreCase)) {
            throw "Unerwarteter Profilpfad: $path"
        }
        if (Test-Path -LiteralPath $path) {
            $item = Get-Item -LiteralPath $path -Force
            if (!$item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                throw "Profilpfad ist kein normaler Ordner: $path"
            }
        }
    }
    return $paths
}

function Move-Profile {
    param([string]$Source, [string]$Destination, [string]$Parent)
    $item = Get-Item -LiteralPath $Source -Force
    if (!$item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -or
        ![string]::Equals($item.Parent.FullName, $Parent, [StringComparison]::OrdinalIgnoreCase) -or
        ![string]::Equals([IO.Path]::GetDirectoryName($Destination), $Parent,
                         [StringComparison]::OrdinalIgnoreCase) -or
        (Test-Path -LiteralPath $Destination)) {
        throw "Profilwechsel angehalten. Quellordner: $Source"
    }
    Rename-Item -LiteralPath $item.FullName -NewName ([IO.Path]::GetFileName($Destination))
}

function Restore-TestProfile {
    param([hashtable]$Paths)
    if (!(Test-Path -LiteralPath $Paths.Journal)) {
        if (Test-Path -LiteralPath $Paths.Original) {
            throw "Originalprofil-Sicherung ohne Statusdatei: $($Paths.Original)"
        }
        return
    }
    $state = [IO.File]::ReadAllText($Paths.Journal)
    if ($state -cnotin '0', '1') { throw "Ungueltige Statusdatei: $($Paths.Journal)" }
    $hasBackup = Test-Path -LiteralPath $Paths.Original
    if ($state -eq '0' -and $hasBackup) {
        throw "Unerwartete Originalprofil-Sicherung: $($Paths.Original)"
    }
    if (($hasBackup -or $state -eq '0') -and (Test-Path -LiteralPath $Paths.Active)) {
        Move-Profile $Paths.Active $Paths.Test $Paths.Parent
    }
    if ($hasBackup) {
        Move-Profile $Paths.Original $Paths.Active $Paths.Parent
    } elseif ($state -eq '1' -and !(Test-Path -LiteralPath $Paths.Active)) {
        throw 'Originalprofil fehlt. Die Statusdatei bleibt zur Wiederherstellung erhalten.'
    }
    Remove-Item -LiteralPath $Paths.Journal
}

function Start-TestProfile {
    param([hashtable]$Paths)
    if ((Test-Path -LiteralPath $Paths.Journal) -or (Test-Path -LiteralPath $Paths.Original)) {
        throw 'Vor dem Start muss die vorige Testsitzung wiederhergestellt werden.'
    }
    $hasOriginal = Test-Path -LiteralPath $Paths.Active
    [IO.File]::WriteAllText($Paths.Journal, [string][int]$hasOriginal)
    if ($hasOriginal) {
        Move-Profile $Paths.Active $Paths.Original $Paths.Parent
    }
    if (Test-Path -LiteralPath $Paths.Test) {
        Move-Profile $Paths.Test $Paths.Active $Paths.Parent
    } else {
        New-Item -ItemType Directory -Path $Paths.Active | Out-Null
    }
}

function Invoke-TestSession {
    param([switch]$Online, [switch]$Editor)

    Assert-GameClosed
    $work = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'DominionModTests\3315'
    $profileParent = Join-Path ([Environment]::GetFolderPath('ApplicationData')) 'TempleGates'
    New-Item -ItemType Directory -Path $work, $profileParent -Force | Out-Null
    $lockPath = Join-Path $work 'session.lock'
    $lock = [IO.File]::Open($lockPath, 'OpenOrCreate', 'ReadWrite', 'None')
    try {
        $paths = Get-ProfilePaths $profileParent (Join-Path $work 'session.state')
        Assert-GameClosed
        Restore-TestProfile $paths

        $session = '--local'
        Write-Host ''
        if ($Online) {
            Write-Host 'Dominion 3315 | Privates Spiel mit festen Effekten'
            $session = '--online'
            $mode = '--apply'
        } elseif ($Editor) {
            Write-Host 'Dominion 3315 | Effekteditor (F8)'
            $mode = '--apply'
        } else {
            Write-Host 'Dominion 3315 | Lokal spielen und Weitergeben'
            Write-Host '1  Messen (erster Test, Originalverhalten)'
            Write-Host '2  Patch anwenden'
            Write-Host '3  Patch mit automatischer Verdopplung der gezogenen Stapel'
            Write-Host 'Q  Beenden'
            $choice = Read-Host 'Testmodus waehlen'
            $mode = switch ($choice) {
                '1' { '--measure' }
                '2' { '--apply' }
                '3' { '--apply-auto' }
                default { return }
            }
        }

        $game = Join-Path $work 'game\Dominion.exe'
        Write-Host 'Die Arbeitskopie wird vorbereitet ...'
        & (Join-Path $PSScriptRoot 'Prepare-TestCopy.ps1')
        $attach = Join-Path $PSScriptRoot 'out\attach.exe'
        $dll = Join-Path $PSScriptRoot 'out\KingdomEffectsPatch.dll'
        $sources = 'patch.cpp', 'attach.cpp', 'working_copy.hpp', 'effects.hpp', 'effect_editor.cpp', 'effect_editor.hpp',
            'effect_editor.rc', 'effect_editor_ids.h', 'card_picker.cpp', 'card_picker.hpp', 'generate_effect_catalog.py', 'build.cmd',
            '..\Documentation\Steam_Spielanleitung.md', '..\Documentation\Card_Properties.json',
            'dialog_skin.cpp', 'dialog_skin.hpp', 'game_resources.cpp', 'game_resources.hpp', 'launcher_test.cpp', 'release_game_test.cpp', 'editor_release_test.cpp', '..\SetGenerator\Dominion-Generator.html',
            '..\SetGenerator\generator_catalog.py', '..\SetGenerator\extract_card_images.py' |
            ForEach-Object { Get-Item -LiteralPath (Join-Path $PSScriptRoot $_) }
        $sources += @(Get-Item -LiteralPath (Join-Path $PSScriptRoot '..\SetGenerator\Dominion-Kartenbilder.json') -ErrorAction SilentlyContinue)
        $newestSource = ($sources | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1).LastWriteTimeUtc
        $binaries = Get-Item -LiteralPath $attach, $dll -ErrorAction SilentlyContinue
        if (@($binaries).Count -ne 2 -or @($binaries | Where-Object { $_.LastWriteTimeUtc -lt $newestSource }).Count) {
            & (Join-Path $PSScriptRoot 'build.cmd')
            if ($LASTEXITCODE -ne 0) { throw 'Die Patch-Dateien konnten nicht gebaut werden.' }
        }

        Assert-GameClosed
        try {
            Start-TestProfile $paths
            Write-Host 'Dominion startet. Bitte auf die Meldung des Starters warten.'
            $process = Start-Process -FilePath $game -WorkingDirectory (Split-Path $game) -WindowStyle Normal -PassThru
            & $attach $session $mode $process.Id $game 'E:\ProgrammeE\Steam\steamapps\common\Dominion\Dominion.exe'
            if ($LASTEXITCODE -ne 0) {
                throw 'Der Patch konnte nicht angehaengt werden. Bitte das Spiel schliessen.'
            }
            if ($Online) {
                Write-Host 'Bereit. In Dominion im Profil den Offline-Modus ausschalten und auf die Verbindung warten.'
                Write-Host 'Danach ein Privates Spiel mit dem Test-Set erstellen.'
            } else {
                Write-Host 'Bereit. In Dominion Lokal spielen waehlen.'
                if ($mode -ne '--measure') {
                    Write-Host 'In der Koenigreich-Auswahl F8 druecken, um die Effekte zu bearbeiten.'
                }
            }
            Write-Host 'Das Originalprofil wird wiederhergestellt, sobald Dominion geschlossen ist.'
            $process.WaitForExit()
            $log = Join-Path ([IO.Path]::GetTempPath()) "DominionKingdomEffects-$($process.Id).log"
            if (Test-Path -LiteralPath $log) {
                $lastLog = Join-Path $work 'letzter-test.log'
                Copy-Item -LiteralPath $log -Destination $lastLog -Force
                Write-Host "Protokoll: $lastLog"
            }
        } finally {
            if (Get-Process Dominion -ErrorAction SilentlyContinue) {
                Write-Host 'Bitte Dominion schliessen, damit das Originalprofil wieder aktiv wird.'
            }
            do {
                while (Get-Process Dominion -ErrorAction SilentlyContinue) {
                    Start-Sleep -Milliseconds 500
                }
                Start-Sleep -Milliseconds 800
            } while (Get-Process Dominion -ErrorAction SilentlyContinue)
            Restore-TestProfile $paths
            Write-Host 'Testprofil gespeichert. Originalprofil wieder aktiv.'
        }
        $null = Read-Host 'Enter zum Schliessen'
    } finally {
        $lock.Dispose()
    }
}

if ($MyInvocation.InvocationName -ne '.') {
    try {
        Invoke-TestSession -Online:$Online -Editor:$Editor
    } catch {
        Write-Host $_.Exception.Message
        exit 1
    }
}
