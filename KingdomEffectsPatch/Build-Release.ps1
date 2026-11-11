param([switch]$SkipBuild, [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ReleaseSupport.ps1')
if (!$SkipBuild) {
    & (Join-Path $PSScriptRoot 'build.cmd') staging
    if ($LASTEXITCODE -ne 0) { throw 'Release-Build fehlgeschlagen.' }
}
$build = Join-Path $PSScriptRoot 'out\staging'
& (Join-Path $build 'effects_test.exe')
if ($LASTEXITCODE -ne 0) { throw 'Effekttests fehlgeschlagen.' }
if (!$OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot ('out\releases\' + (Get-Date -Format 'yyyyMMdd-HHmmss')) }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
Assert-NormalDirectory $OutputDirectory
$package = Join-Path $OutputDirectory 'KingdomEffectsPatch-v0.1.0'
if (Test-Path -LiteralPath $package) { throw 'Ausgabeordner bereits vorhanden. Bitte einen neuen Ausgabeordner waehlen.' }
New-Item -ItemType Directory -Path (Join-Path $package 'bin') -Force | Out-Null
# Explicit allowlist: no source game, user data, logs, test fixtures or build intermediates.
foreach ($file in 'Start-Mod.ps1', 'ReleaseSupport.ps1', 'Dominion-mit-Mod-starten.cmd', 'README.md', 'RELEASE-NOTES.md') {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $package
}
foreach ($file in 'attach.exe', 'KingdomEffectsPatch.dll') {
    Copy-Item -LiteralPath (Join-Path $build $file) -Destination (Join-Path $package 'bin')
}
foreach ($folder in 'card-faces', 'menu-art') {
    $destination = Join-Path $package "bin\$folder"
    New-Item -ItemType Directory -Path $destination | Out-Null
    $images = @(Get-ChildItem -LiteralPath (Join-Path $build $folder) -File -Filter '*.png')
    if (!$images.Count) { throw "Grafiken fehlen: $folder" }
    foreach ($image in $images) { Copy-Item -LiteralPath $image.FullName -Destination $destination }
}
$files = [ordered]@{}
foreach ($file in Get-ChildItem -LiteralPath $package -File -Recurse | Sort-Object FullName) {
    $name = $file.FullName.Substring($package.Length + 1).Replace('\', '/')
    $files[$name] = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
}
$revision = & git -C $PSScriptRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Git-Quellstand konnte nicht ermittelt werden.' }
$changes = @(& git -C $PSScriptRoot status --porcelain)
if ($LASTEXITCODE -ne 0) { throw 'Git-Arbeitsstand konnte nicht ermittelt werden.' }
$manifest = [ordered]@{ version = '0.1.0'; gameBuild = '3315'; steamBuild = '17683350'; exeSha256 = $script:SupportedExeHash; sourceCommit = $revision; sourceDirty = ($changes.Count -ne 0); files = $files }
[IO.File]::WriteAllText((Join-Path $package 'package.json'), ($manifest | ConvertTo-Json -Depth 4), $script:Utf8)
Assert-ModPackage $package
$archive = Join-Path $OutputDirectory 'KingdomEffectsPatch-v0.1.0-windows.zip'
Compress-Archive -LiteralPath $package -DestinationPath $archive -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'SHA256SUMS.txt'), "$hash  $([IO.Path]::GetFileName($archive))`r`n", $script:Utf8)
Write-Host "Release-Paket: $archive"
Write-Host "Pruefsumme: $hash"
