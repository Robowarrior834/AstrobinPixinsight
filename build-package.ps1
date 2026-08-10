#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Builds the AstroBin CSV Generator update package for PixInsight.

.DESCRIPTION
    Creates a zip package with the proper directory structure for PixInsight's
    update repository system, calculates the SHA1 checksum, and updates the
    updates.xri manifest file.

.EXAMPLE
    .\build-package.ps1
#>

$ErrorActionPreference = "Stop"

# Extract version from the JS source file
$SourceScript = "AstroBinCSVGenerator.js"
$SourceContent = [System.IO.File]::ReadAllText($SourceScript)
$VersionMatch = [regex]::Match($SourceContent, '#define\s+VERSION\s+"([^"]+)"')
if (-not $VersionMatch.Success) {
    Write-Error "Could not extract VERSION from $SourceScript"
    exit 1
}
$ScriptVersion = $VersionMatch.Groups[1].Value
$PackageName = "astrobin-$ScriptVersion"
$BuildDir = "build"
$PackageFile = "$PackageName.zip"
$XriFile = "updates\updates.xri"

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

# Update updates.xri with the correct SHA1
Write-Host "Updating $XriFile..." -ForegroundColor Yellow
$XriContent = [System.IO.File]::ReadAllText($XriFile)
$XriContent = $XriContent -replace 'sha1="[^"]*"', "sha1=`"$Sha1`""

# Update filename to match the new package
$XriContent = $XriContent -replace 'fileName="astrobin-[^"]*\.zip"', "fileName=`"$PackageFile`""

# Update title with new version
$XriContent = $XriContent -replace '<title>AstroBin CSV Generator v[^<]*</title>', "<title>AstroBin CSV Generator v$ScriptVersion</title>"

# Update release date to today
$Today = Get-Date -Format "yyyyMMdd"
$XriContent = $XriContent -replace 'releaseDate="[^"]*"', "releaseDate=`"$Today`""

# Preserve CRLF line endings (required by PixInsight)
$XriContent = $XriContent -replace "\r?\n", "`r`n"
[System.IO.File]::WriteAllText($XriFile, $XriContent, [System.Text.UTF8Encoding]::new($false))

Write-Host "Updated $XriFile with SHA1 and release date" -ForegroundColor Green
Write-Host ""

# Clean up build directory
Remove-Item -Recur -Force $BuildDir

Write-Host "Build complete!" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. RE-SIGN updates\updates.xri with PixInsight (Script > Code Sign)"
Write-Host "     using your CPD .xssk keys file. IMPORTANT: this must be done"
Write-Host "     AFTER this build, since the SHA1/releaseDate were just updated."
Write-Host "     Skipping this leaves a stale signature (PixInsight shows"
Write-Host "     'Signature: <* unavailable *>')."
Write-Host "  2. Commit updates/ directory to git"
Write-Host "  3. Push to GitHub"
Write-Host "  4. Users can add the repository URL in PixInsight"
