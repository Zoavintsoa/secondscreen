[CmdletBinding()]
param(
    [string]$PackageDirectory = (Join-Path (Split-Path -Parent $PSScriptRoot) 'artifacts\windows-driver'),
    [switch]$EnableTestSigning
)

$ErrorActionPreference = 'Stop'

if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Run this script from an elevated PowerShell session.'
}

$infPath = Join-Path $PackageDirectory 'SecondScreenIddCx.inf'
if (-not (Test-Path $infPath)) {
    throw "Driver INF not found: $infPath. Run scripts/build-windows-driver.ps1 first."
}

if ($EnableTestSigning) {
    & bcdedit /set testsigning on
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to enable test-signing. Secure Boot or local policy may prevent it.'
    }
    Write-Warning 'Test-signing was enabled. Reboot Windows before installing a test-signed package.'
}

$devcon = Get-ChildItem -Path (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Tools') -Recurse -Filter devcon.exe -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\x64\\' } | Select-Object -First 1 -ExpandProperty FullName
if (-not $devcon) {
    throw 'devcon.exe from the Windows Driver Kit is required to create the Root\SecondScreenVirtualDisplay device.'
}

& pnputil /add-driver $infPath
if ($LASTEXITCODE -ne 0) {
    throw "pnputil failed with exit code $LASTEXITCODE."
}

& $devcon install $infPath 'Root\SecondScreenVirtualDisplay'
if ($LASTEXITCODE -ne 0) {
    throw "devcon could not create the root-enumerated device (exit code $LASTEXITCODE)."
}

Write-Host 'Driver package installed. Open Settings > System > Display and verify SecondScreen Virtual Display before starting the host.'
