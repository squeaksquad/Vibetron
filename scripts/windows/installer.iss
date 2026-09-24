; Windows installer for Vibetron VT-369 (VST3 + Standalone). Built by .github/workflows/windows.yml:
;   iscc /DAppVersion=<version> /DArtefacts=<build>\Vibetron_artefacts\Release scripts\windows\installer.iss

#ifndef AppVersion
  #error Pass /DAppVersion=<version>
#endif
#ifndef Artefacts
  #error Pass /DArtefacts=<path to Vibetron_artefacts\Release>
#endif

#define Product "Vibetron VT-369"

[Setup]
AppId={{6B1E3C52-8F4D-4A2E-9C77-5D0A1F3B9E21}
AppName={#Product}
AppVersion={#AppVersion}
AppPublisher=Vibetron
AppPublisherURL=https://github.com/squeaksquad/Vibetron
DefaultDirName={autopf}\Vibetron
DefaultGroupName=Vibetron
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir=..\..\dist
OutputBaseFilename=Vibetron-VT-369-{#AppVersion}-Windows
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#Product}

[Types]
Name: "full"; Description: "VST3 plug-in and standalone app"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in"; Types: full custom
Name: "standalone"; Description: "Standalone app"; Types: full

[Files]
Source: "{#Artefacts}\VST3\{#Product}.vst3\*"; DestDir: "{commoncf64}\VST3\{#Product}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\{#Product}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#Product}"; Filename: "{app}\{#Product}.exe"; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#Product}.vst3"
