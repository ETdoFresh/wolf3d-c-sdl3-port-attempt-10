# publish.ps1 -- assemble a runnable wolf3d/ build into publish/.
#
# Builds Release (skips if -SkipBuild), then copies wolf3d.exe, SDL3.dll,
# and the game-data files from assets/wl6/ into publish/. Run from any
# directory; the script resolves paths relative to the repo root.

param(
    [switch]$SkipBuild,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$RepoRoot   = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildDir   = Join-Path $RepoRoot 'build'
$AssetsDir  = Join-Path $RepoRoot 'assets\wl6'
$PublishDir = Join-Path $RepoRoot 'publish'

Write-Host "publish.ps1 -- repo: $RepoRoot"

if ($Clean -and (Test-Path $PublishDir)) {
    Write-Host "Cleaning $PublishDir"
    Remove-Item -Recurse -Force $PublishDir
}

if (-not $SkipBuild) {
    if (-not (Test-Path $BuildDir)) {
        Write-Host "Configuring CMake (Release)..."
        & cmake -B $BuildDir -DCMAKE_BUILD_TYPE=Release
        if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
    }
    Write-Host "Building Release..."
    & cmake --build $BuildDir --config Release -j4
    # Post-build copy steps can fail when a previous wolf3d.exe is still
    # running; the main link target is what matters, so we don't error here.
    if (-not (Test-Path (Join-Path $BuildDir 'Release\wolf3d.exe'))) {
        throw "build did not produce wolf3d.exe"
    }
}

New-Item -ItemType Directory -Force -Path $PublishDir | Out-Null

$exeSrc = Join-Path $BuildDir 'Release\wolf3d.exe'
$dllSrc = Join-Path $BuildDir 'Release\SDL3.dll'
if (-not (Test-Path $dllSrc)) {
    $dllSrc = Join-Path $BuildDir '_deps\sdl3-build\Release\SDL3.dll'
}

Write-Host "Copying binaries..."
Copy-Item $exeSrc $PublishDir -Force
Copy-Item $dllSrc $PublishDir -Force

Write-Host "Copying game data from $AssetsDir..."
Get-ChildItem -Path $AssetsDir -Filter '*.WL6' -File | ForEach-Object {
    Copy-Item $_.FullName $PublishDir -Force
}
$gamepal = Join-Path $AssetsDir 'GAMEPAL.BIN'
if (Test-Path $gamepal) {
    Copy-Item $gamepal $PublishDir -Force
}

Write-Host ""
Write-Host ("Published to {0} :" -f $PublishDir)
Get-ChildItem $PublishDir | Where-Object { -not $_.PSIsContainer } |
    Sort-Object Name |
    Format-Table -AutoSize Name, Length, LastWriteTime
