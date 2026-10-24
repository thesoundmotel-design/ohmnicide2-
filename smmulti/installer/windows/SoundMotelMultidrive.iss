[Setup]
AppName=Sound Motel Multidrive
AppVersion=1.0.0
AppPublisher=The Sound Motel
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
CreateUninstallRegKey=yes
UninstallDisplayName=Sound Motel Multidrive (VST3)
OutputDir=..\..\dist
OutputBaseFilename=SoundMotelMultidrive-1.0.0-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes

[Files]
Source: "..\..\build\SoundMotelMultidrive_artefacts\Release\VST3\Sound Motel Multidrive.vst3\*"; DestDir: "{commoncf64}\VST3\Sound Motel Multidrive.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion
