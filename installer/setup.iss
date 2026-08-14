; Inno Setup script for the Jeopardy Game.
; Build with: ISCC.exe installer\setup.iss
; Produces dist\JeopardyGame-Setup.exe. Requires a Release build already at
; build-release\JeopardyGame.exe (see README.md for the CMake commands).

#define MyAppName "Jeopardy Game"
#define MyAppVersion "1.0.1"
#define MyAppPublisher "robertja81"
#define MyAppURL "https://github.com/robertja81/jeopardy-game"
#define MyAppExeName "JeopardyGame.exe"

[Setup]
; Fixed AppId so future installer versions upgrade in place rather than
; installing side-by-side. Do not change this once released.
AppId={{36138CE1-9033-4513-818B-272F16260DA8}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\JeopardyGame
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=JeopardyGame-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "..\build-release\JeopardyGame.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dictionaries\Round\*.json"; DestDir: "{app}\Dictionaries\Round"; Flags: ignoreversion
Source: "..\dictionaries\FinalJeopardy\*.json"; DestDir: "{app}\Dictionaries\FinalJeopardy"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; DestName: "README.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
