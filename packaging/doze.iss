#ifndef PackageDir
 #error PackageDir is required
#endif
#ifndef OutputDir
 #define OutputDir "output"
#endif
#ifndef AppVersion
 #define AppVersion "1.0.0"
#endif
[Setup]
AppId={{E3C2F82B-2C94-44C1-8912-93BD7301CB4D}
AppName=Doze
AppVersion={#AppVersion}
AppPublisher=CustomInfo
AppPublisherURL=https://custominfo.it/
AppSupportURL=https://custominfo.it/
DefaultDirName={localappdata}\Programs\Doze
DefaultGroupName=Doze
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
OutputDir={#OutputDir}
OutputBaseFilename=Doze-Setup-{#AppVersion}
SetupIconFile=..\assets\doze-windows.ico
UninstallDisplayIcon={app}\Doze.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
DisableProgramGroupPage=yes
ChangesAssociations=yes

[Languages]
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
italian.ContextMenu=Mostra Doze con Shift + tasto destro su desktop e cartelle
english.ContextMenu=Show Doze with Shift + right-click on the desktop and folders
italian.DesktopIcon=Crea un collegamento sul desktop
english.DesktopIcon=Create a desktop shortcut
italian.RemovePreferences=Eliminare anche le preferenze e la cache di Doze? Scegli No per conservarle.
english.RemovePreferences=Also delete Doze preferences and cache? Choose No to keep them.

[Tasks]
Name: "contextmenu"; Description: "{cm:ContextMenu}"
Name: "desktopicon"; Description: "{cm:DesktopIcon}"; Flags: unchecked

[Files]
Source: "{#PackageDir}\Doze.exe"; DestDir: "{app}"; Flags: ignoreversion

Source: "{#PackageDir}\README.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PackageDir}\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion

[InstallDelete]
Type: files; Name: "{app}\WebView2Loader.dll"

[Icons]
Name: "{autoprograms}\Doze"; Filename: "{app}\Doze.exe"
Name: "{autodesktop}\Doze"; Filename: "{app}\Doze.exe"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Classes\DesktopBackground\shell\CustomInfo.Doze"; ValueType: string; ValueName: ""; ValueData: "Doze"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\DesktopBackground\shell\CustomInfo.Doze"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\Doze.exe"",1"; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\DesktopBackground\shell\CustomInfo.Doze"; ValueType: string; ValueName: "Extended"; ValueData: ""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\DesktopBackground\shell\CustomInfo.Doze"; ValueType: string; ValueName: "Position"; ValueData: "Bottom"; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\DesktopBackground\shell\CustomInfo.Doze\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Doze.exe"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\Background\shell\CustomInfo.Doze"; ValueType: string; ValueName: ""; ValueData: "Doze"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\Background\shell\CustomInfo.Doze"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\Doze.exe"",1"; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\Background\shell\CustomInfo.Doze"; ValueType: string; ValueName: "Extended"; ValueData: ""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\Background\shell\CustomInfo.Doze"; ValueType: string; ValueName: "Position"; ValueData: "Bottom"; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\Background\shell\CustomInfo.Doze\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Doze.exe"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\shell\CustomInfo.Doze"; ValueType: string; ValueName: ""; ValueData: "Doze"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\shell\CustomInfo.Doze"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\Doze.exe"",1"; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\shell\CustomInfo.Doze"; ValueType: string; ValueName: "Extended"; ValueData: ""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\shell\CustomInfo.Doze"; ValueType: string; ValueName: "Position"; ValueData: "Bottom"; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\Directory\shell\CustomInfo.Doze\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Doze.exe"""; Tasks: contextmenu

[Run]
Filename: "{app}\Doze.exe"; Description: "Doze"; Flags: nowait postinstall skipifsilent unchecked

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then begin
    RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Classes\DesktopBackground\shell\CustomInfo.Doze');
    RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Classes\Directory\Background\shell\CustomInfo.Doze');
    RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Classes\Directory\shell\CustomInfo.Doze');
  end;
end;
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    if not UninstallSilent then
      if MsgBox(CustomMessage('RemovePreferences'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
        DelTree(ExpandConstant('{localappdata}\CustomInfo\Doze'), True, True, True);
end;
