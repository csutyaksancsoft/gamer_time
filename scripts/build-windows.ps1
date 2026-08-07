#requires -Version 5.1

[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [string]$BuildDirectory = "build-windows",

    [switch]$SkipPackage
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

if ($env:OS -ne "Windows_NT") {
    throw "This script must be run in Windows PowerShell, not Linux, macOS, or WSL."
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$buildPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $BuildDirectory))
$sdlPath = Join-Path $repoRoot "external\SDL"
$sdlCMake = Join-Path $sdlPath "CMakeLists.txt"
$sdlRevision = "a157d96de87fce84f06e60f4049dd96b8a6993e3"
$enetPath = Join-Path $repoRoot "external\enet"
$enetCMake = Join-Path $enetPath "CMakeLists.txt"
$enetRevision = "v1.3.18"

function Assert-LastExitCode {
    param([string]$Action)

    if ($LASTEXITCODE -ne 0) {
        throw "$Action failed with exit code $LASTEXITCODE."
    }
}

Write-Host "Checking build prerequisites..." -ForegroundColor Cyan

$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakeCommand) {
    throw "CMake was not found in PATH. Install CMake 4.2 or newer, then reopen PowerShell."
}

$cmakeVersionText = (& cmake --version | Select-Object -First 1) -replace '^cmake version\s+', ''
$cmakeVersion = [version]$cmakeVersionText
if ($cmakeVersion -lt [version]"4.2.0") {
    throw "CMake $cmakeVersion is too old for Visual Studio 2026. Install CMake 4.2 or newer."
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "Visual Studio Installer was not found. Install Visual Studio 2026 Community."
}

$vsInstall = & $vswhere `
    -latest `
    -products * `
    -version '[18.0,19.0)' `
    -requires Microsoft.VisualStudio.Workload.NativeDesktop `
    -property installationPath

if (-not $vsInstall) {
    throw "Visual Studio 2026 with the Desktop development with C++ workload was not found."
}

if (-not $env:VULKAN_SDK) {
    throw "VULKAN_SDK is not set. Install the x64 Vulkan SDK, then reopen PowerShell."
}

$vulkanHeader = Join-Path $env:VULKAN_SDK "Include\vulkan\vulkan.h"
$vulkanLibrary = Join-Path $env:VULKAN_SDK "Lib\vulkan-1.lib"
$shaderCompiler = Join-Path $env:VULKAN_SDK "Bin\glslangValidator.exe"

foreach ($requiredFile in @($vulkanHeader, $vulkanLibrary, $shaderCompiler)) {
    if (-not (Test-Path $requiredFile)) {
        throw "The Vulkan SDK is incomplete or VULKAN_SDK is incorrect. Missing: $requiredFile"
    }
}

Write-Host "  CMake:        $cmakeVersion" -ForegroundColor Green
Write-Host "  Visual Studio: $vsInstall" -ForegroundColor Green
Write-Host "  Vulkan SDK:    $env:VULKAN_SDK" -ForegroundColor Green

if (-not (Test-Path $sdlCMake)) {
    Write-Host "Downloading pinned SDL source (no Git required)..." -ForegroundColor Cyan

    $temporaryPath = Join-Path ([System.IO.Path]::GetTempPath()) ("gamer-time-sdl-" + [guid]::NewGuid())
    $archivePath = Join-Path $temporaryPath "SDL.zip"
    $extractPath = Join-Path $temporaryPath "extracted"
    $archiveUrl = "https://github.com/libsdl-org/SDL/archive/$sdlRevision.zip"

    try {
        New-Item -ItemType Directory -Path $temporaryPath | Out-Null
        New-Item -ItemType Directory -Path $extractPath | Out-Null
        Invoke-WebRequest -Uri $archiveUrl -OutFile $archivePath
        Expand-Archive -Path $archivePath -DestinationPath $extractPath

        $expandedRoot = Get-ChildItem -Path $extractPath -Directory | Select-Object -First 1
        if (-not $expandedRoot) {
            throw "The downloaded SDL archive did not contain a source directory."
        }

        New-Item -ItemType Directory -Path $sdlPath -Force | Out-Null
        Copy-Item -Path (Join-Path $expandedRoot.FullName "*") -Destination $sdlPath -Recurse -Force
    }
    finally {
        if (Test-Path $temporaryPath) {
            Remove-Item -Path $temporaryPath -Recurse -Force
        }
    }

    if (-not (Test-Path $sdlCMake)) {
        throw "SDL download completed, but $sdlCMake is still missing."
    }

    Write-Host "  SDL:          downloaded revision $sdlRevision" -ForegroundColor Green
}
else {
    Write-Host "  SDL:          existing source found" -ForegroundColor Green
}

if (-not (Test-Path $enetCMake)) {
    Write-Host "Downloading pinned ENet source (no Git required)..." -ForegroundColor Cyan
    $temporaryPath = Join-Path ([System.IO.Path]::GetTempPath()) ("gamer-time-enet-" + [guid]::NewGuid())
    $archivePath = Join-Path $temporaryPath "enet.zip"
    $extractPath = Join-Path $temporaryPath "extracted"
    try {
        New-Item -ItemType Directory -Path $extractPath -Force | Out-Null
        Invoke-WebRequest -Uri "https://github.com/lsalzman/enet/archive/refs/tags/$enetRevision.zip" -OutFile $archivePath
        Expand-Archive -Path $archivePath -DestinationPath $extractPath
        $expandedRoot = Get-ChildItem -Path $extractPath -Directory | Select-Object -First 1
        if (-not $expandedRoot) { throw "The ENet archive was empty." }
        New-Item -ItemType Directory -Path $enetPath -Force | Out-Null
        Copy-Item -Path (Join-Path $expandedRoot.FullName "*") -Destination $enetPath -Recurse -Force
    }
    finally {
        if (Test-Path $temporaryPath) { Remove-Item -Path $temporaryPath -Recurse -Force }
    }
    if (-not (Test-Path $enetCMake)) { throw "ENet download completed but CMakeLists.txt is missing." }
}
else {
    Write-Host "  ENet:         existing source found" -ForegroundColor Green
}

Write-Host "Configuring Visual Studio 2026 x64 build..." -ForegroundColor Cyan
& cmake `
    -S $repoRoot `
    -B $buildPath `
    -G "Visual Studio 18 2026" `
    -A x64 `
    -DGT_BUILD_CLIENT=ON `
    -DGT_BUILD_SERVER=OFF `
    -DBUILD_TESTING=OFF
Assert-LastExitCode "CMake configuration"

Write-Host "Building $Configuration client..." -ForegroundColor Cyan
& cmake --build $buildPath --config $Configuration --target gamer_time --parallel
Assert-LastExitCode "$Configuration build"

$executable = Join-Path $buildPath "$Configuration\gamer_time.exe"
if (-not (Test-Path $executable)) {
    throw "The build succeeded, but the expected executable was not found: $executable"
}

Write-Host "Executable created:" -ForegroundColor Green
Write-Host "  $executable"

if (-not $SkipPackage) {
    Write-Host "Creating portable ZIP..." -ForegroundColor Cyan
    & cmake --build $buildPath --config $Configuration --target package
    Assert-LastExitCode "Portable ZIP packaging"

    $package = Join-Path $buildPath "gamer-time-windows-x64.zip"
    if (-not (Test-Path $package)) {
        throw "Packaging succeeded, but the expected ZIP was not found: $package"
    }

    Write-Host "Portable client created:" -ForegroundColor Green
    Write-Host "  $package"
}

Write-Host "Done." -ForegroundColor Green
