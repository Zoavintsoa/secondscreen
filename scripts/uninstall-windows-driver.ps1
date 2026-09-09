[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Run this script from an elevated PowerShell session.'
}

$devcon = Get-ChildItem -Path (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Tools') -Recurse -Filter devcon.exe -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\x64\\' } | Select-Object -First 1 -ExpandProperty FullName
if ($devcon) {
    & $devcon remove 'Root\SecondScreenVirtualDisplay'
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to remove the root device with devcon (exit code $LASTEXITCODE)."
    }
}

$drivers = & pnputil /enum-drivers
$publishedName = ($drivers | Select-String -Pattern '^Published Name\s*:\s*(oem\d+\.inf)$' -Context 0,6 | Where-Object { $_.Context.PostContext -match 'SecondScreen Virtual Display' } | ForEach-Object { $_.Matches[0].Groups[1].Value } | Select-Object -First 1)
if ($publishedName) {
    & pnputil /delete-driver $publishedName /uninstall /force
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to remove driver package $publishedName (exit code $LASTEXITCODE)."
    }
}

Write-Host 'SecondScreen driver device and package removal completed.'
