$ErrorActionPreference = 'Stop'
$source = 'E:\ProgrammeE\Steam\steamapps\common\Dominion'
$manifest = 'E:\ProgrammeE\Steam\steamapps\appmanifest_1131620.acf'
$expected = '56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877'
$target = Join-Path $env:LOCALAPPDATA 'DominionModTests\3315\game'

if (Get-Process Dominion -ErrorAction SilentlyContinue) {
    throw 'Bitte Dominion vor dem Kopieren vollständig schließen.'
}
if ((Get-Content -LiteralPath (Join-Path $source 'build_number.txt') -Raw).Trim() -ne '3315' -or
    (Get-Content -LiteralPath $manifest -Raw) -notmatch '(?m)^\s*"buildid"\s+"17683350"') {
    throw 'Die installierte Version entspricht nicht Spielbuild 3315 und Steam-Build 17683350.'
}
if ((Get-FileHash -LiteralPath (Join-Path $source 'Dominion.exe')).Hash -ne $expected) {
    throw 'Der EXE-Hash der Installation weicht von der geprüften Version ab.'
}
if (!(Test-Path -LiteralPath $target)) {
    $staging = "$target.preparing-$([Guid]::NewGuid().ToString('N'))"
    New-Item -ItemType Directory -Path $staging -Force | Out-Null
    Get-ChildItem -LiteralPath $source -Force | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $staging -Recurse
    }
    if ((Get-FileHash -LiteralPath (Join-Path $staging 'Dominion.exe')).Hash -ne $expected) {
        throw 'Der Hash der neu erstellten Arbeitskopie stimmt nicht.'
    }
    $candidate = Get-Item -LiteralPath $staging
    if (![string]::Equals($candidate.Parent.FullName, (Split-Path $target), [StringComparison]::OrdinalIgnoreCase) -or
        ($candidate.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Unerwarteter Ordner fuer die Arbeitskopie: $staging"
    }
    Rename-Item -LiteralPath $candidate.FullName -NewName 'game'
}
$directory = Get-Item -LiteralPath $target
if (!$directory.PSIsContainer -or ($directory.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
    throw "Die Arbeitskopie muss ein eigener Ordner sein: $target"
}
$exe = Join-Path $target 'Dominion.exe'
if ((Get-FileHash -LiteralPath $exe).Hash -ne $expected) {
    throw 'Der Hash der Arbeitskopie stimmt nicht.'
}
foreach ($file in Get-ChildItem -LiteralPath $source -File -Force) {
    $copy = Get-Item -LiteralPath (Join-Path $target $file.Name)
    if ($copy.PSIsContainer -or $copy.Length -ne $file.Length -or
        ($copy.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Datei der Arbeitskopie fehlt oder weicht ab: $($file.Name)"
    }
}
Write-Output "Arbeitskopie bereit: $exe"
