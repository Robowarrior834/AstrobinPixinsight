#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Copies the AstroBin CSV Generator script into the local PixInsight
    installation for testing.

.DESCRIPTION
    Copies AstroBinCSVGenerator.js (and its .xsgn signature if present) into
    C:\Program Files\PixInsight\src\scripts. The destination directory is
    created if it does not exist. The script automatically relaunches itself
    elevated (UAC prompt) when needed, because the default destination is
    under Program Files.

.EXAMPLE
    .\deploy-to-pixinsight.ps1

.EXAMPLE
    .\deploy-to-pixinsight.ps1 -Destination "C:\PixInsight\src\scripts"
#>

param(
    [Parameter(Mandatory = $false)]
    [string]$SourceScript = "AstroBinCSVGenerator.js",

    [Parameter(Mandatory = $false)]
    [string]$Destination = "C:\Program Files\PixInsight\src\scripts"
)

$ErrorActionPreference = "Stop"

# If not running as Administrator, relaunch this script elevated (UAC prompt)
if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "Elevating to Administrator to write under Program Files..." -ForegroundColor Yellow
    $Shell = (Get-Process -Id $PID).Path
    try {
        Start-Process -FilePath $Shell -Verb RunAs -Wait `
            -ArgumentList "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "`"$PSCommandPath`"", "`"-SourceScript`"", "`"$SourceScript`"", "`"-Destination`"", "`"$Destination`""
    }
    catch {
        Write-Error "Elevation was cancelled. This script needs Administrator privileges."
        exit 1
    }
    exit
}

if (-not (Test-Path $SourceScript)) {
    Write-Error "Source script '$SourceScript' not found in current directory."
    exit 1
}

if (-not (Test-Path $Destination)) {
    Write-Host "Destination '$Destination' does not exist - creating it..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
}

try {
    Write-Host "Copying $SourceScript -> $Destination" -ForegroundColor Yellow
    Copy-Item $SourceScript -Destination $Destination -Force

    $SignatureFile = "AstroBinCSVGenerator.xsgn"
    if (Test-Path $SignatureFile) {
        Write-Host "Copying $SignatureFile -> $Destination" -ForegroundColor Yellow
        Copy-Item $SignatureFile -Destination $Destination -Force
        Write-Host "Signature file copied." -ForegroundColor Green
    }
    else {
        Write-Warning "$SignatureFile not found - copied script without signature. Re-sign the script first (Scripts -> Development -> Code Sign)."
    }
}
catch [System.UnauthorizedAccessException] {
    Write-Error "Access denied. Run this script from an elevated (Administrator) PowerShell session."
    exit 1
}
catch [System.Exception] {
    Write-Error $_.Exception.Message
    exit 1
}

Write-Host ""
Write-Host "Deployed. Restart PixInsight, then run Scripts -> Utilities -> AstroBin CSV Generator" -ForegroundColor Green
