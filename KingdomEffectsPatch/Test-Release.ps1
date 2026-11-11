param([string]$PackageDirectory = '', [string]$ArchivePath = '',
      [Parameter(Mandatory=$true)][string]$SourceGameDirectory)
$ErrorActionPreference = 'Stop'
$scratch = Join-Path ([IO.Path]::GetTempPath()) ('DominionReleaseTests-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $scratch | Out-Null
if ($ArchivePath) {
    $unpacked = Join-Path $scratch ('Release entpackt ' + [char]0x00E4)
    Expand-Archive -LiteralPath $ArchivePath -DestinationPath $unpacked
    $PackageDirectory = Join-Path $unpacked 'KingdomEffectsPatch-v0.1.0'
}
if (!$PackageDirectory) { throw 'Bitte -ArchivePath oder -PackageDirectory angeben.' }
. (Join-Path $PackageDirectory 'ReleaseSupport.ps1')
function Assert-True { param([bool]$Condition, [string]$Message); if (!$Condition) { throw $Message } }
function Assert-Throws {
    param([scriptblock]$Action, [string]$Message)
    $failed = $false
    try { & $Action | Out-Null } catch { $failed = $true }
    Assert-True $failed $Message
}
Assert-SupportedGame $SourceGameDirectory
Assert-ModPackage $PackageDirectory
Assert-True (!@(Get-ChildItem -LiteralPath $PackageDirectory -Recurse -File | Where-Object { $_.Extension -match '^\.(png|jpe?g|webp|bmp|gif|ico|svg)$' }).Count) 'Das Release darf keine Grafikdateien enthalten.'

# Check every shipped file, not just the runtime binary check.
$manifest = [IO.File]::ReadAllText((Join-Path $PackageDirectory 'package.json')) | ConvertFrom-Json
$files = @(Get-ChildItem -LiteralPath $PackageDirectory -File -Recurse)
Assert-True ($files.Count -eq @($manifest.files.PSObject.Properties).Count + 1) 'Paket enthaelt unerwartete Dateien.'
foreach ($entry in $manifest.files.PSObject.Properties) {
    $path = Join-Path $PackageDirectory $entry.Name
    Assert-True ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -eq $entry.Value) "Paketdatei geaendert: $($entry.Name)"
}
Assert-True (!@($files | Where-Object { $_.Name -match '^(Dominion\.exe|profiles.*\.xml)$|\.(xpack|log|obj|pdb|lib|exp)$' }).Count) 'Private Dateien oder Spieldateien im Paket.'

# Backup rotation uses only generated profiles; real user data is never written.
$profileRoot = Join-Path $scratch 'profile'
$backups = Join-Path $scratch 'backups'
New-Item -ItemType Directory -Path $profileRoot | Out-Null
$profile = Join-Path $profileRoot 'profiles.xml'
Backup-ModProfile $profile $backups
Assert-True (!(Test-Path -LiteralPath $backups)) 'Fehlendes Profil erzeugt keine Sicherung.'
foreach ($number in 1..8) {
    [IO.File]::WriteAllText($profile, "<test>$number</test>", $script:Utf8)
    Backup-ModProfile $profile $backups
}
$newest = Join-Path $backups 'profiles-1.xml'
Assert-True (@(Get-ChildItem -LiteralPath $backups -File).Count -eq 3) 'Sicherungen nicht auf drei begrenzt.'
Assert-True ([IO.File]::ReadAllText($newest) -eq '<test>8</test>') 'Neueste Sicherung falsch.'
Assert-True ([IO.File]::ReadAllText((Join-Path $backups 'profiles-3.xml')) -eq '<test>6</test>') 'Rotation falsch.'
$timestamp = (Get-Item -LiteralPath $newest).LastWriteTimeUtc
Backup-ModProfile $profile $backups
Assert-True ((Get-Item -LiteralPath $newest).LastWriteTimeUtc -eq $timestamp) 'Unveraendertes Profil wurde neu gesichert.'
Assert-True ([IO.File]::ReadAllText($profile) -eq '<test>8</test>') 'Originalprofil wurde veraendert.'
[IO.File]::WriteAllText($profile, '<test>9</test>', $script:Utf8)
$held = [IO.File]::Open((Join-Path $backups 'profile.pending'), 'OpenOrCreate', 'ReadWrite', 'None')
try { Assert-Throws { Backup-ModProfile $profile $backups } 'Gesperrte Sicherung wurde nicht abgelehnt.' }
finally { $held.Dispose() }
Assert-True ([IO.File]::ReadAllText($newest) -eq '<test>8</test>') 'Fehler hat die neueste Sicherung veraendert.'

# Discovery and validation on alternate paths, including a Unicode directory name.
$unicodeName = 'Steam Bibliothek ' + [char]0x00E4
$steamRoot = Join-Path $scratch 'Steam'
$library = Join-Path $scratch $unicodeName
$source = Join-Path $library 'steamapps\common\Dominion'
New-Item -ItemType Directory -Path $source, (Join-Path $steamRoot 'steamapps') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $SourceGameDirectory 'Dominion.exe') -Destination $source
[IO.File]::WriteAllText((Join-Path $source 'build_number.txt'), '3315')
# This fixture only tests discovery and validation; it is never launched.
[IO.File]::WriteAllText((Join-Path $source 'Dominion.xpack'), 'fixture, not game content')
$escaped = $library.Replace('\', '\\')
[IO.File]::WriteAllText((Join-Path $steamRoot 'steamapps\libraryfolders.vdf'), '"libraryfolders" { "0" { "path" "' + $escaped + '" } }', $script:Utf8)
$found = @(Get-SteamDominionCandidates @($steamRoot))
Assert-True ($found.Count -eq 1 -and $found[0] -eq $source) 'Steam-Bibliothek nicht gefunden.'
$state = Join-Path $scratch 'state'
New-Item -ItemType Directory -Path $state | Out-Null
Assert-True ((Resolve-ModInstallation $state $source) -eq $source) 'Expliziter Pfad nicht akzeptiert.'
Assert-True ((Resolve-ModInstallation $state) -eq $source) 'Gespeicherter Pfad nicht wiederverwendet.'
[IO.File]::WriteAllText((Join-Path $source 'build_number.txt'), '9999')
Assert-Throws { Assert-SupportedGame $source } 'Falscher Spielbuild wurde akzeptiert.'
[IO.File]::WriteAllText((Join-Path $source 'build_number.txt'), '3315')
[IO.File]::WriteAllText((Join-Path $source 'Dominion.exe'), 'bad exe')
Assert-Throws { Assert-SupportedGame $source } 'Falscher EXE-Hash wurde akzeptiert.'
Copy-Item -LiteralPath (Join-Path $SourceGameDirectory 'Dominion.exe') -Destination (Join-Path $source 'Dominion.exe') -Force
$starterText = [IO.File]::ReadAllText((Join-Path $PackageDirectory 'Start-Mod.ps1'))
Assert-True ($starterText.Contains('--installed --apply')) 'Release startet nicht im Installationsmodus.'
Assert-True ($starterText -notmatch 'Prepare-ModWorkingCopy|Start-TestProfile|Copy-Item') 'Release kopiert das Spiel oder wechselt das Profil.'

# Session lock behavior is independent of the foreground desktop.
$lockPath = Join-Path $scratch 'session.lock'
$held = [IO.File]::Open($lockPath, 'OpenOrCreate', 'ReadWrite', 'None')
try {
    Assert-Throws { $other = [IO.File]::Open($lockPath, 'OpenOrCreate', 'ReadWrite', 'None'); $other.Dispose() } 'Doppelstart nicht abgewehrt.'
} finally { $held.Dispose() }

# A broken release is rejected before discovery, backups or a game launch.
$broken = Join-Path $scratch 'broken-package'
New-Item -ItemType Directory -Path $broken | Out-Null
Assert-Throws { Assert-ModPackage $broken } 'Fehlendes Manifest wurde akzeptiert.'
Copy-Item -LiteralPath (Join-Path $PackageDirectory 'package.json') -Destination $broken
Assert-Throws { Assert-ModPackage $broken } 'Fehlende Binaerdateien wurden akzeptiert.'
Write-Host 'PASS: Paket, Sicherungen, Pfade, Steam-Erkennung, Installationsmodus, Fehlerfaelle und Sitzungssperre.'
Write-Host "Isolierte Testdaten: $scratch"
