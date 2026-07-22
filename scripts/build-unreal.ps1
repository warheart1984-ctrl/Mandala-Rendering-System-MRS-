#Requires -Version 5.1
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Root "unreal\GovernedUnrealProject\GovernedUnrealProject.uproject"

$UeRoots = @(
  "${env:ProgramFiles}\Epic Games\UE_5.8",
  "${env:ProgramFiles}\Epic Games\UE_5.5",
  "${env:ProgramFiles}\Epic Games\UE_5.4"
)

$UeRoot = $UeRoots | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $UeRoot) {
  throw "Unreal Engine not found. Install UE 5.8 or set UE_ROOT manually."
}

$WindowsSdkRoot = "${env:ProgramFiles(x86)}\Windows Kits\10\Include"
if (-not (Test-Path $WindowsSdkRoot)) {
  Write-Warning @"
Windows 10 SDK not found — Unreal C++ builds require Visual Studio 2022 with:
  - Workload: Desktop development with C++
  - Windows 10 SDK (10.0.19041.0 or newer)

Install via Visual Studio Installer, then re-run this script.
You can still open the .uproject in Unreal Editor; it will prompt to build when SDK is available.
"@
}

$BuildBat = Join-Path $UeRoot "Engine\Build\BatchFiles\Build.bat"
if (-not (Test-Path $BuildBat)) {
  throw "Build.bat not found at $BuildBat"
}

& $PSScriptRoot\setup-unreal.ps1

Write-Host "Building GovernedUnrealProjectEditor (Development Win64)..."
Write-Host "Project: $Project"

& $BuildBat GovernedUnrealProjectEditor Win64 Development "-Project=$Project" -WaitMutex

if ($LASTEXITCODE -ne 0) {
  throw "Unreal build failed with exit code $LASTEXITCODE"
}

Write-Host "Build succeeded."
