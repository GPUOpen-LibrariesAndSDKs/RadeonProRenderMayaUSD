#define AppVersionString '0.1.0'

[Setup]
AppName=RPRMayaUSDHdRPR
AppVersion={#AppVersionString}
WizardStyle=modern
DefaultDirName="{autopf64}\RPRMayaUSDHdRPR"
DefaultGroupName=RPRMayaUSDHdRPR
ChangesEnvironment=no
OutputDir=.
OutputBaseFilename=RPRMayaUSDHdRPR_Setup_{#AppVersionString}
SetupIconFile=.\ico\AMD.ico
//LicenseFile=LICENSE.txt

[Files]
//Source: "..\LICENSE.txt"; DestDir: "{app}";
Source: "..\Build_RPRUsdInstall\*"; DestDir: "{app}"; Flags: recursesubdirs

[InstallDelete]
Type: filesandordirs; Name: "{app}"

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
var
  MayaVersion : String;

procedure InitializeWizard();
begin
  WizardForm.LicenseAcceptedRadio.Checked := True;
  MayaVersion := '2023';
end;

procedure ModifyMayaEnvFile(maya_version : String; action : String);
var
  resultCode: Integer;
begin
  //MsgBox('action: ' + action);
  //MsgBox('ExpandConstant('{app}\hdRPR');

  if Exec(ExpandConstant('{app}\MayaEnvModifier.exe'), 
      ExpandConstant(action + ' ' + '"{app}\hdRPR"'), '', SW_SHOW, ewWaitUntilTerminated, resultCode)
  then 
  begin
    if not (resultCode = 0) then
      MsgBox('Internal Setup Error: Maya.env file has not been modified! ' + maya_version, mbInformation, MB_OK);
  end;

end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  mayaModulePath : String;
  resultCode : Integer;
begin
  if CurStep = ssPostInstall then
  begin
    ModifyMayaEnvFile(MayaVersion, '-add');

    mayaModulePath := ExpandConstant('{commoncf64}\Autodesk Shared\modules\maya\' + MayaVersion);

    if Exec(ExpandConstant('{app}\rprUsdModModifier.exe'), 
        ExpandConstant('"{app}\rprUsd\rprUsd.mod" "{app}\rprUsd"'), '', SW_SHOW, ewWaitUntilTerminated, resultCode)
    then 
    begin
      if not (resultCode = 0) then
        MsgBox('Internal Setup Error: rprUsd.mod file has not been modified! ' + MayaVersion, mbInformation, MB_OK);
    end;
    
    if DirExists(mayaModulePath) then
      if not FileCopy(ExpandConstant('{app}\rprUsd\rprUsd.mod'), mayaModulePath + '\rprUsd.mod', false)
      then
        MsgBox('Setup Error: RprUsd.mod file could not be copied to Maya''s modules directory! ' + MayaVersion, mbInformation, MB_OK);     

  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  path : String;
begin
  if CurUninstallStep = usUninstall then
  begin
    path := ExpandConstant('{commoncf64}\Autodesk Shared\modules\maya\' + MayaVersion + '\rprUsd.mod');
    DeleteFile(path);

    ModifyMayaEnvFile(MayaVersion, '-remove');
  end
end;