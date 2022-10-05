#define AppVersionString '0.1.7'

[Setup]
AppName=RPRMayaUSD
AppVersion={#AppVersionString}
WizardStyle=modern
DefaultDirName="{autopf64}\RPRMayaUSD"
DefaultGroupName=RPRMayaUSD
ChangesEnvironment=no
OutputDir=.
OutputBaseFilename=RPRMayaUSD_Setup_{#AppVersionString}
SetupIconFile=.\ico\AMD.ico
//LicenseFile=LICENSE.txt

[Files]
//Source: "..\LICENSE.txt"; DestDir: "{app}";
Source: "..\Build\*"; DestDir: "{app}"; Flags: recursesubdirs
//Source: "..\Build\maya-usd\build\install\RelWithDebInfo\RPRMayaUSD.mod"; DestDir: "{app}\maya-usd\build\install\RelWithDebInfo\"; Flags: recursesubdirs
//Source: "..\Build\ModModifier.exe"; DestDir: "{app}"; Flags: recursesubdirs
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

procedure ModifyModFileAndCopy(maya_version : String);
var
  ResultCode: Integer;
  Path : String;
begin
  if Exec(ExpandConstant('{app}\ModModifier.exe'), 
      ExpandConstant('"{app}\' + maya_version + '\maya-usd\build\install\RelWithDebInfo\RPRMayaUSD.mod" "{app}\' + maya_version + '"') , '', SW_SHOW, ewWaitUntilTerminated, ResultCode)
  then 
  begin
    if not (ResultCode = 0) then
      MsgBox('Internal Setup Error: RPRMayaUSD.mod file has not been modified ! ' + maya_version, mbInformation, MB_OK);
  end;

  Path := ExpandConstant('{commoncf64}\Autodesk Shared\modules\maya\' + maya_version);

  if DirExists(Path) then
    if not FileCopy(ExpandConstant('{app}\' + maya_version + '\maya-usd\build\install\RelWithDebInfo\RPRMayaUSD.mod'), Path + '\RPRMayaUSD.mod', false)
    then
      MsgBox('Setup Error: RPRMayaUSD.mod file could not be copied to Maya''s modules directory! ' + maya_version, mbInformation, MB_OK);     
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    ModifyModFileAndCopy('2022');
    ModifyModFileAndCopy('2023');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    DeleteFile(ExpandConstant('{commoncf64}\Autodesk Shared\modules\maya\2022\RPRMayaUSD.mod'));
    DeleteFile(ExpandConstant('{commoncf64}\Autodesk Shared\modules\maya\2023\RPRMayaUSD.mod'));
  end
end;