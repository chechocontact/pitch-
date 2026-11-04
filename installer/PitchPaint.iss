; Inno Setup script: installs PitchPaint.vst3 for all users on Windows 11 (x64).
; Build:  iscc /DAppVersion=0.2.0 /DVst3Dir=..\build\PitchPaint_artefacts\Release\VST3 installer\PitchPaint.iss
#ifndef AppVersion
  #define AppVersion "0.2.0"
#endif
#ifndef Vst3Dir
  #define Vst3Dir "..\build\PitchPaint_artefacts\Release\VST3"
#endif

[Setup]
AppId={{6C1F5E0A-3B7D-4C55-9E21-7A0F4B1D2C90}
AppName=PitchPaint
AppVersion={#AppVersion}
AppPublisher=PitchPaint contributors
AppPublisherURL=https://github.com/saitekii/PitchPaint
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
LicenseFile=..\LICENSE
OutputDir=..\dist
OutputBaseFilename=PitchPaint-{#AppVersion}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=PitchPaint (VST3)
; Never restart: FL Studio just needs a plugin rescan.
RestartIfNeededByRun=no
CloseApplications=no

[Messages]
FinishedLabel=PitchPaint was installed to the VST3 folder.%n%nIn FL Studio: open Plugin database, click "Find installed plugins" (make sure C:\Program Files\Common Files\VST3 is in the scan list), then add PitchPaint as an instrument.

[Files]
; A .vst3 built by JUCE on Windows is a folder bundle (Contents\x86_64-win\PitchPaint.vst3).
Source: "{#Vst3Dir}\PitchPaint.vst3\*"; DestDir: "{commoncf64}\VST3\PitchPaint.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\LICENSE"; DestDir: "{commoncf64}\VST3\PitchPaint.vst3\Contents"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{commoncf64}\VST3\PitchPaint.vst3\Contents"; Flags: ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\PitchPaint.vst3"
