[Setup]
AppName=RPRMayaUSD
AppVersion=0.1.0
WizardStyle=modern
DefaultDirName="{autopf64}\RPRMayaUSD"
DefaultGroupName=RPRMayaUSD
ChangesEnvironment=no
OutputDir=.
OutputBaseFilename=RPRMayaUSD_Setup
SetupIconFile=.\ico\AMD.ico
//LicenseFile=LICENSE.txt

[Files]
//Source: "..\LICENSE.txt"; DestDir: "{app}";
Source: "..\Build\maya-usd\build\install\RelWithDebInfo\RPRMayaUSD.mod"; DestDir: "{app}"; Flags: recursesubdirs
//Source: ".\binary\windows\inst\VC_redist.x64.exe"; DestDir: "{app}";
//Source: ".\binary\windows\inst\CheckRprCompatibility.exe"; DestDir: "{app}";

[InstallDelete]
Type: filesandordirs; Name: "{app}"
//Type: filesandordirs; Name: "{userappdata}\Autodesk\ApplicationPlugins\UsdConvertor"
//Type: filesandordirs; Name: "{%USERPROFILE}\.usdview"

[UninstallDelete]
//Type: filesandordirs; Name: "{%USERPROFILE}\.usdview"

[Registry]
//Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName: "HDRPR_CACHE_PATH_OVERRIDE"; \
//ValueData: "{userappdata}\RPRMayaUSD"; Flags: preservestringtype uninsdeletevalue
//Root: HKCU; Subkey: "Software\RPR\USDViewer\"; ValueType:string; ValueName: "UsdViewerExec"; \
//    ValueData: "{app}\RPRViewer.exe"; Flags: preservestringtype uninsdeletekey

[Run]
//Filename: "{app}\VC_redist.x64.exe"; Parameters: "/quiet /norestart";

[Code]
procedure InitializeWizard();
begin
  WizardForm.LicenseAcceptedRadio.Checked := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
//    CheckRprCompatibility();
  end;
end;