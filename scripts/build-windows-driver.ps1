[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $repoRoot 'native\windows\driver\SecondScreenIddCx.vcxproj'

if (-not (Test-Path $projectPath)) {
    throw "Driver project not found: $projectPath"
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw 'Visual Studio with the Windows Driver Kit is required. vswhere.exe was not found.'
}

$msbuildPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuildPath -or -not (Test-Path $msbuildPath)) {
    throw 'MSBuild was not found in the selected Visual Studio installation.'
}

$wdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
if (-not (Test-Path (Join-Path $wdkRoot 'Include'))) {
    throw 'Windows Driver Kit 10/11 headers were not found. Install a WDK matching the Windows SDK.'
}

# WDK 10.0.28000's MSBuild InfVerif target currently fails on the GitHub
# Windows 2025 image because it resolves the x86 DLL from the wrong directory.
# Disable only that MSBuild validation target; Inf2Cat is still run explicitly
# below to validate and generate the driver package catalog.
$msbuildProperties = @(
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform",
    '/p:EnableInfVerif=false',
    '/p:RunApiValidator=false'
)

& $msbuildPath $projectPath @msbuildProperties /t:Rebuild /m
if ($LASTEXITCODE -ne 0) {
    throw "Driver build failed with exit code $LASTEXITCODE."
}

$outputDirectory = Join-Path $repoRoot "native\windows\driver\$Platform\$Configuration"
$driverBinary = Join-Path $outputDirectory 'SecondScreenIddCx.dll'
if (-not (Test-Path $driverBinary)) {
    throw "MSBuild succeeded but the UMDF driver binary was not found: $driverBinary"
}

# Inf2Cat is an x86 WDK tool; prefer the matching x86 binary explicitly.
$inf2catCandidates = @(
    (Join-Path $wdkRoot 'bin\10.0.28000.0\x86\Inf2Cat.exe'),
    (Join-Path $wdkRoot 'bin\x86\Inf2Cat.exe')
)
$inf2cat = $inf2catCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $inf2cat) {
    $inf2cat = Get-ChildItem -Path (Join-Path $wdkRoot 'bin') -Recurse -Filter Inf2Cat.exe -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\x86\\Inf2Cat\.exe$' } |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $inf2cat) {
    throw 'Inf2Cat.exe was not found in the x86 WDK tools. The driver package catalog was not generated.'
}

$packageDirectory = Join-Path $repoRoot 'artifacts\windows-driver'
New-Item -ItemType Directory -Path $packageDirectory -Force | Out-Null
Copy-Item $driverBinary (Join-Path $packageDirectory 'SecondScreenIddCx.dll') -Force
Copy-Item (Join-Path $repoRoot 'native\windows\driver\SecondScreenIddCx.inf') (Join-Path $packageDirectory 'SecondScreenIddCx.inf') -Force

Push-Location $packageDirectory
try {
    & $inf2cat /driver:$packageDirectory /os:10_X64,11_X64
    if ($LASTEXITCODE -ne 0) {
        throw "Inf2Cat failed with exit code $LASTEXITCODE."
    }
} finally {
    Pop-Location
}

Write-Host "Driver package created: $packageDirectory"
