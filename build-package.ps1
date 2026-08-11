#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Builds the AstroBin CSV Generator update package for PixInsight.

.DESCRIPTION
    Creates a zip package with the proper directory structure for PixInsight's
    update repository system. The package file name is taken from a variable at
    the top of the script; updates.xri is never modified.

.EXAMPLE
    .\build-package.ps1
#>

$ErrorActionPreference = "Stop"

# Zip package file name to create (keep in sync with updates\updates.xri)
$PackageFile = "astrobin-1.2.3.zip"

# Extract version from the package file name (e.g. astrobin-1.2.3.zip)
$VersionMatch = [regex]::Match($PackageFile, '-(\d+\.\d+\.\d+)\.zip$')
if (-not $VersionMatch.Success) {
    Write-Error "Could not extract version from package file name '$PackageFile'"
    exit 1
}
$ScriptVersion = $VersionMatch.Groups[1].Value
$SourceScript = "AstroBinCSVGenerator.js"
$BuildDir = "build"

Write-Host "Building AstroBin CSV Generator package v$ScriptVersion" -ForegroundColor Cyan
Write-Host ""

# Clean previous build
if (Test-Path $BuildDir) {
    Remove-Item -Recurse -Force $BuildDir
}

# Create package directory structure
$PackageDir = "$BuildDir\src\scripts"
New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null

# Copy script to package location
Write-Host "Copying $SourceScript..." -ForegroundColor Yellow
Copy-Item $SourceScript -Destination $PackageDir

# Copy signed script to package location (if present)
$SignatureFile = "AstroBinCSVGenerator.xsgn"
if (Test-Path $SignatureFile) {
    Write-Host "Copying $SignatureFile..." -ForegroundColor Yellow
    Copy-Item $SignatureFile -Destination $PackageDir
}
else {
    Write-Warning "$SignatureFile not found - script will be packaged unsigned"
}

# Create zip package in updates directory
Write-Host "Creating $PackageFile..." -ForegroundColor Yellow
$PackagePath = "updates\$PackageFile"
if (Test-Path $PackagePath) {
    Remove-Item -Force $PackagePath
}
Compress-Archive -Path "$BuildDir\src" -DestinationPath $PackagePath -CompressionLevel Optimal

# Calculate SHA1 checksum
Write-Host "Calculating SHA1 checksum..." -ForegroundColor Yellow
$Hash = Get-FileHash -Path $PackagePath -Algorithm SHA1
$Sha1 = $Hash.Hash.ToLower()

Write-Host ""
Write-Host "Package: $PackagePath" -ForegroundColor Green
Write-Host "SHA1:    $Sha1" -ForegroundColor Green
Write-Host ""

# Clean up build directory
Remove-Item -Recur -Force $BuildDir

Write-Host "Build complete!" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Update the sha1 attribute in updates\updates.xri to match the"
Write-Host "     checksum above, then RE-SIGN updates\updates.xri with PixInsight"
Write-Host "     (Script > Code Sign) using your CPD .xssk keys file."
Write-Host "  2. Commit updates/ directory to git"
Write-Host "  3. Push to GitHub"
Write-Host "  4. Users can add the repository URL in PixInsight"
