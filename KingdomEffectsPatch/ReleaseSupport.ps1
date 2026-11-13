# Shared by the release launcher and its isolated tests. No work runs on import.
$script:SupportedExeHash = '56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877'
$script:Utf8 = New-Object System.Text.UTF8Encoding($false)

function Assert-NormalDirectory {
    param([string]$Path)
    $directory = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if (!$directory.PSIsContainer -or ($directory.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Kein normaler Ordner: $Path"
    }
    # Reject redirected ancestors too, especially for copy and backup destinations.
    while ($directory) {
        if ($directory.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Umgeleiteter Ordner wird nicht verwendet: $($directory.FullName)"
        }
        $directory = $directory.Parent
    }
}

function Assert-ModGameClosed {
    if (Get-Process Dominion -ErrorAction SilentlyContinue) {
        throw 'Bitte zuerst alle Dominion-Fenster schliessen.'
    }
}

function Assert-SupportedGame {
    param([string]$Directory)
    Assert-NormalDirectory $Directory
    $exe = Join-Path $Directory 'Dominion.exe'
    $item = Get-Item -LiteralPath $exe -Force -ErrorAction Stop
    if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Die Spiel-EXE darf keine Verknuepfung sein.' }
    if ((Get-Content -LiteralPath (Join-Path $Directory 'build_number.txt') -Raw).Trim() -ne '3315' -or
        (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -ne $script:SupportedExeHash) {
        throw 'Nicht unterstuetzte Dominion-Version. v0.1.1 benoetigt den geprueften Spielbuild 3315. Es wurde nichts gepatcht.'
    }
    if (!(Test-Path -LiteralPath (Join-Path $Directory 'Dominion.xpack') -PathType Leaf)) {
        throw 'Dominion.xpack fehlt in der Spielinstallation.'
    }
}

function Get-SteamDominionCandidates {
    param([string[]]$SteamRoots = @())
    if (!$SteamRoots.Count) {
        foreach ($key in 'HKCU:\Software\Valve\Steam', 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam') {
            $properties = Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
            if ($properties) {
                foreach ($name in 'SteamPath', 'InstallPath') {
                    if ($properties.PSObject.Properties[$name]) { $SteamRoots += $properties.$name }
                }
            }
        }
        if (${env:ProgramFiles(x86)}) { $SteamRoots += Join-Path ${env:ProgramFiles(x86)} 'Steam' }
    }
    $libraries = @($SteamRoots)
    foreach ($root in $SteamRoots) {
        $vdf = Join-Path $root 'steamapps\libraryfolders.vdf'
        if (Test-Path -LiteralPath $vdf -PathType Leaf) {
            foreach ($match in [regex]::Matches([IO.File]::ReadAllText($vdf), '"(?:path|[0-9]+)"\s+"([^"]+)"')) {
                $candidate = $match.Groups[1].Value.Replace('\\', '\')
                if ([IO.Path]::IsPathRooted($candidate)) { $libraries += $candidate }
            }
        }
    }
    @($libraries | Select-Object -Unique | ForEach-Object {
        $candidate = Join-Path $_ 'steamapps\common\Dominion'
        if (Test-Path -LiteralPath (Join-Path $candidate 'Dominion.exe') -PathType Leaf) {
            (Get-Item -LiteralPath $candidate).FullName
        }
    } | Sort-Object -Unique)
}

function Resolve-ModInstallation {
    param([string]$StateRoot, [string]$GameDirectory = '')
    $config = Join-Path $StateRoot 'settings.json'
    if (!$GameDirectory -and (Test-Path -LiteralPath $config -PathType Leaf)) {
        $saved = [IO.File]::ReadAllText($config) | ConvertFrom-Json
        if ($saved.PSObject.Properties['gameDirectory'] -and
            (Test-Path -LiteralPath (Join-Path $saved.gameDirectory 'Dominion.exe') -PathType Leaf)) {
            $GameDirectory = $saved.gameDirectory
        }
    }
    if (!$GameDirectory) {
        $candidates = @(Get-SteamDominionCandidates)
        if ($candidates.Count -eq 1) { $GameDirectory = $candidates[0] }
        else {
            if ($candidates.Count) { Write-Host ($candidates -join "`r`n") }
            $GameDirectory = (Read-Host 'Bitte den Dominion-Spielordner aus Steam > Verwalten > Lokale Dateien durchsuchen eingeben').Trim().Trim('"')
        }
    }
    if (!$GameDirectory) { throw 'Kein Spielordner angegeben.' }
    $GameDirectory = (Get-Item -LiteralPath $GameDirectory -ErrorAction Stop).FullName
    Assert-SupportedGame $GameDirectory
    $temporary = "$config.new"
    [IO.File]::WriteAllText($temporary, (@{ gameDirectory = $GameDirectory } | ConvertTo-Json), $script:Utf8)
    Move-Item -LiteralPath $temporary -Destination $config -Force
    $GameDirectory
}

function Backup-ModProfile {
    param([string]$ProfilePath, [string]$BackupDirectory)
    if (!(Test-Path -LiteralPath $ProfilePath)) { return }
    $profile = Get-Item -LiteralPath $ProfilePath -Force
    Assert-NormalDirectory $profile.DirectoryName
    if ($profile.PSIsContainer -or ($profile.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Das Profil ist keine normale Datei.'
    }
    New-Item -ItemType Directory -Path $BackupDirectory -Force | Out-Null
    Assert-NormalDirectory $BackupDirectory
    # Fixed slots guarantee a bounded number of backups, including after interruptions.
    $slots = @(1..3 | ForEach-Object { Join-Path $BackupDirectory "profiles-$_.xml" })
    foreach ($slot in $slots) {
        if (Test-Path -LiteralPath $slot) {
            $file = Get-Item -LiteralPath $slot -Force
            if ($file.PSIsContainer -or ($file.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                throw "Ungueltiger Sicherungspfad: $slot"
            }
        }
    }
    $hash = (Get-FileHash -LiteralPath $ProfilePath -Algorithm SHA256).Hash
    if ((Test-Path -LiteralPath $slots[0]) -and
        (Get-FileHash -LiteralPath $slots[0] -Algorithm SHA256).Hash -eq $hash) { return }
    $pending = Join-Path $BackupDirectory 'profile.pending'
    if (Test-Path -LiteralPath $pending) {
        $file = Get-Item -LiteralPath $pending -Force
        if ($file.PSIsContainer -or ($file.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Ungueltige temporaere Sicherung.' }
    }
    Copy-Item -LiteralPath $ProfilePath -Destination $pending -Force
    if ((Get-FileHash -LiteralPath $pending -Algorithm SHA256).Hash -ne $hash -or
        (Get-FileHash -LiteralPath $ProfilePath -Algorithm SHA256).Hash -ne $hash) {
        throw 'Profilsicherung nicht konsistent. Bitte das Spiel schliessen und erneut starten.'
    }
    if (Test-Path -LiteralPath $slots[1]) { Copy-Item -LiteralPath $slots[1] -Destination $slots[2] -Force }
    if (Test-Path -LiteralPath $slots[0]) { Copy-Item -LiteralPath $slots[0] -Destination $slots[1] -Force }
    Move-Item -LiteralPath $pending -Destination $slots[0] -Force
}

function Assert-ModPackage {
    param([string]$PackageRoot)
    $manifestPath = Join-Path $PackageRoot 'package.json'
    if (!(Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'package.json fehlt. Bitte das vollstaendige Release-ZIP entpacken.' }
    $manifest = [IO.File]::ReadAllText($manifestPath) | ConvertFrom-Json
    if ($manifest.version -ne '0.1.1' -or $manifest.gameBuild -ne '3315') { throw 'Unbekanntes Release-Paket.' }
    foreach ($name in 'attach.exe', 'KingdomEffectsPatch.dll') {
        $path = Join-Path $PackageRoot "bin\$name"
        $expected = $manifest.files.PSObject.Properties["bin/$name"]
        if (!$expected -or !(Test-Path -LiteralPath $path -PathType Leaf) -or
            (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $expected.Value) {
            throw "Release-Datei fehlt oder wurde veraendert: $name. Bitte das ZIP erneut entpacken."
        }
    }
}
