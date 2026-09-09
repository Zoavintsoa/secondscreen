#define AppName "SecondScreen"
#ifndef AppVersion
  #define AppVersion "0.1.0-dev"
#endif
#ifndef BuildOutput
  #error BuildOutput must point to the prepared Windows staging directory.
#endif

[Setup]
AppId={{A2A14A0F-756B-497A-9495-58936D3F8B82}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=SecondScreen
DefaultDirName={autopf}\SecondScreen
DefaultGroupName=SecondScreen
OutputDir={#BuildOutput}\installer
OutputBaseFilename=SecondScreen-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
UninstallDisplayName=SecondScreen
RestartIfNeededByRun=no
DisableProgramGroupPage=yes

[Files]
Source: "{#BuildOutput}\app\SecondScreenHost.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildOutput}\app\SecondScreenDriverInstaller.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildOutput}\driver\SecondScreenIddCx.inf"; DestDir: "{app}\driver"; Flags: ignoreversion
Source: "{#BuildOutput}\driver\SecondScreenIddCx.cat"; DestDir: "{app}\driver"; Flags: ignoreversion
Source: "{#BuildOutput}\driver\SecondScreenIddCx.dll"; DestDir: "{app}\driver"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\SecondScreen Host"; Filename: "{app}\SecondScreenHost.exe"
Name: "{autodesktop}\SecondScreen Host"; Filename: "{app}\SecondScreenHost.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{app}\SecondScreenDriverInstaller.exe"; Parameters: "install ""{app}\driver\SecondScreenIddCx.inf"""; StatusMsg: "Installing SecondScreen Virtual Display Driver..."; Flags: runhidden waituntilterminated
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall add rule name=""SecondScreen Host"" dir=in action=allow program=""{app}\SecondScreenHost.exe"" enable=yes profile=private"; StatusMsg: "Configuring the private-network firewall rule..."; Flags: runhidden waituntilterminated
Filename: "{app}\SecondScreenHost.exe"; Description: "Launch SecondScreen Host"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\SecondScreenDriverInstaller.exe"; Parameters: "uninstall"; StatusMsg: "Removing SecondScreen Virtual Display device..."; Flags: runhidden waituntilterminated
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=""SecondScreen Host"""; Flags: runhidden waituntilterminated
