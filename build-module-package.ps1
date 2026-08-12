#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Packages the built AstroBin CSV Generator process module for distribution.

.DESCRIPTION
    Collects the compiled module DLL, docs and license into a release folder
    under dist\ and zips it. The DLL must already be built and, for end users
    to install it, signed with PixInsight module signing keys.

.EXAMPLE
    .\build-module-package.ps1
#>

$ErrorActionPreference = "Stop"

# Keep in sync with module\AstroBinCSVGeneratorModule.cpp MODULE_VERSION_*
$ModuleVersion = "1.2.5"
$DllPath = "C:\PCL\bin\x64\AstroBinCSVGenerator-pxm.dll"
$DistDir = "dist"
$ReleaseDir = "$DistDir\AstroBinCSVGenerator-$ModuleVersion"
$ZipPath = "$DistDir\AstroBinCSVGenerator-$ModuleVersion.zip"

if (-not (Test-Path $DllPath)) {
    Write-Error "Module DLL not found at $DllPath - run C:\PCL\build-module.cmd first"
    exit 1
}

# Clean previous release
if (Test-Path $ReleaseDir) {
    Remove-Item -Recurse -Force $ReleaseDir
}
New-Item -ItemType Directory -Path "$ReleaseDir\bin" -Force | Out-Null

Write-Host "Building AstroBin CSV Generator module package v$ModuleVersion" -ForegroundColor Cyan
Write-Host ""

Write-Host "Copying module DLL..." -ForegroundColor Yellow
Copy-Item $DllPath -Destination "$ReleaseDir\bin"

foreach ($File in "README.md", "docs\MODULE.md", "LICENSE") {
    if (Test-Path $File) {
        Write-Host "Copying $File..." -ForegroundColor Yellow
        Copy-Item $File -Destination $ReleaseDir
    }
}

if (Test-Path $ZipPath) {
    Remove-Item -Force $ZipPath
}
Compress-Archive -Path "$ReleaseDir\*" -DestinationPath $ZipPath -CompressionLevel Optimal

Write-Host ""
Write-Host "Package: $ZipPath" -ForegroundColor Green
Write-Host "DLL SHA256: $((Get-FileHash $DllPath -Algorithm SHA256).Hash)" -ForegroundColor Green
Write-Host ""
Write-Host "REMINDER: PixInsight will not load an unsigned module." -ForegroundColor Yellow
Write-Host "Sign $DllPath with your PixInsight module signing keys before shipping this package."
Write-Host ""
