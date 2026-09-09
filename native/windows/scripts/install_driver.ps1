# SecondScreen Windows Virtual Display Driver Installer
# Automated test-certificate generation, driver signing, and installation
# Must be executed with Administrative Privileges

Param(
    [switch]$Uninstall,
    [string]$DriverPath = "$PSScriptRoot\..\driver\SecondScreenIddCx.inf",
    [string]$CertName = "SecondScreen Development Test Certificate"
)

Write-Host "=========================================================" -ForegroundColor Cyan
Write-Host "   SecondScreen IddCx Virtual Display Driver Installer   " -ForegroundColor Cyan
Write-Host "=========================================================" -ForegroundColor Cyan

# 1. Verify Administrator Privileges
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Error "Administrator privileges required. Please re-run this script in an elevated PowerShell session."
    exit 1
}

# 2. Handle Driver Uninstallation
if ($Uninstall) {
    Write-Host "[*] Removing SecondScreen Virtual Display Driver package..." -ForegroundColor Yellow
    pnputil /delete-driver $DriverPath /uninstall /force
    Write-Host "[✓] Driver package removed from Driver Store." -ForegroundColor Green
    exit 0
}

# 3. Verify and Enable Windows Test-Signing Mode
Write-Host "[*] Step 1: Checking Windows Test-Signing mode..."
$testSigning = bcdedit /enum "{current}" | Select-String "testsigning\s+Yes"
if (-not $testSigning) {
    Write-Warning "Windows Test-Signing mode is currently OFF."
    Write-Host "[*] Enabling Test-Signing via bcdedit..." -ForegroundColor Cyan
    bcdedit /set testsigning on
    Write-Host "[!] IMPORTANT: If test-signing was just enabled for the first time, a system reboot is required." -ForegroundColor Yellow
} else {
    Write-Host "[✓] Windows Test-Signing mode is ACTIVE." -ForegroundColor Green
}

# 4. Generate or Verify Self-Signed Test Certificate
Write-Host "[*] Step 2: Checking development test certificate..."
$cert = Get-ChildItem -Path Cert:\LocalMachine\Root | Where-Object { $_.Subject -match $CertName }

if (-not $cert) {
    Write-Host "[*] Creating self-signed code signing certificate: '$CertName'..." -ForegroundColor Cyan
    $cert = New-SelfSignedCertificate `
        -Type CodeSigningCert `
        -Subject "CN=$CertName" `
        -CertStoreLocation "Cert:\LocalMachine\My" `
        -NotAfter (Get-Date).AddYears(5)

    # Trust the certificate in Trusted Root Certification Authorities
    $rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("Root", "LocalMachine")
    $rootStore.Open("ReadWrite")
    $rootStore.Add($cert)
    $rootStore.Close()

    # Also add to Trusted Publishers
    $pubStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("TrustedPublisher", "LocalMachine")
    $pubStore.Open("ReadWrite")
    $pubStore.Add($cert)
    $pubStore.Close()

    Write-Host "[✓] Certificate created and trusted in Root and TrustedPublisher stores." -ForegroundColor Green
} else {
    Write-Host "[✓] Existing test certificate found in Root store." -ForegroundColor Green
}

# 5. Sign Driver Package if SignTool is available
$signTool = (Get-ChildItem -Path "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe" -ErrorAction SilentlyContinue | Select-Object -First 1).FullName
$driverDllPath = "$PSScriptRoot\..\driver\x64\Release\SecondScreenIddCx.dll"
$driverCatPath = "$PSScriptRoot\..\driver\SecondScreenIddCx.cat"

if ($signTool -and (Test-Path $driverDllPath)) {
    Write-Host "[*] Step 3: Digitally signing driver binaries with SignTool..."
    & $signTool sign /v /s "My" /n "$CertName" /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 $driverDllPath
    if (Test-Path $driverCatPath) {
        & $signTool sign /v /s "My" /n "$CertName" /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 $driverCatPath
    }
    Write-Host "[✓] Driver binaries digitally signed." -ForegroundColor Green
}

# 6. Install Driver into Windows Driver Store
Write-Host "[*] Step 4: Adding driver package to Windows Driver Store via pnputil..."
$pnputilResult = pnputil /add-driver $DriverPath /install

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=========================================================" -ForegroundColor Green
    Write-Host " [✓] SecondScreen Virtual Display Installed Successfully!" -ForegroundColor Green
    Write-Host "=========================================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next Steps to Verify Display 2:" -ForegroundColor Cyan
    Write-Host " 1. Open Windows Settings -> System -> Display."
    Write-Host " 2. Confirm 'SecondScreen Virtual Display (Display 2)' appears."
    Write-Host " 3. Select 'Extend these displays' and configure desired resolution."
} else {
    Write-Error "pnputil installation failed with exit code $LASTEXITCODE."
    Write-Host "Troubleshooting:" -ForegroundColor Yellow
    Write-Host " - Ensure the driver was compiled in Visual Studio (x64 Release)."
    Write-Host " - Verify bcdedit /set testsigning on was executed and machine rebooted."
}
