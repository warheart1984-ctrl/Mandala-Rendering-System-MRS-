#Requires -Version 5.1
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$ProjectDir = Join-Path $Root "unreal\GovernedUnrealProject"
$PluginLink = Join-Path $ProjectDir "Plugins\GovernedEngine"
$PluginSrc = Join-Path $Root "unreal\GovernedEnginePlugin"

New-Item -ItemType Directory -Force -Path (Join-Path $ProjectDir "Plugins") | Out-Null

if (-not (Test-Path $PluginLink)) {
  cmd /c mklink /J "`"$PluginLink`"" "`"$PluginSrc`""
  Write-Host "Created plugin junction: $PluginLink"
} else {
  Write-Host "Plugin junction already exists."
}

Write-Host "Unreal project ready: $ProjectDir\GovernedUnrealProject.uproject"
