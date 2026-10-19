#ifndef MyVersion
  #define MyVersion "1.0.2"
#endif

[Setup]
AppId={{B3D8F2A7-41C6-4E9A-8F15-7A2C9E64D0B3}
AppName=PitchShifter
AppVersion={#MyVersion}
AppPublisher=Ticco Ross
DefaultDirName={autopf}\Ticco Ross\PitchShifter
DefaultGroupName=PitchShifter
DisableProgramGroupPage=yes
OutputBaseFilename=PitchShifter-{#MyVersion}-Windows-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
LicenseFile=License
SourceDir=..\..

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full custom
Name: "app"; Description: "Standalone application"; Types: full custom
Name: "presets"; Description: "Presets and user manual"; Types: full custom

[Files]
Source: "stage\VST3\PitchShifter.vst3\*"; DestDir: "{commoncf64}\VST3\PitchShifter.vst3"; Components: vst3; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "stage\PitchShifter.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "stage\Presets\*.xml"; DestDir: "{app}\Presets"; Components: presets; Flags: ignoreversion
Source: "stage\PitchShifter-Manual.pdf"; DestDir: "{app}"; Components: presets; Flags: ignoreversion

[Icons]
Name: "{group}\PitchShifter"; Filename: "{app}\PitchShifter.exe"; Components: app
Name: "{group}\PitchShifter manual"; Filename: "{app}\PitchShifter-Manual.pdf"; Components: presets
Name: "{group}\Uninstall PitchShifter"; Filename: "{uninstallexe}"
