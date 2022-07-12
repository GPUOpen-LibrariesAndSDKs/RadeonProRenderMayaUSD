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
procedure InitializeWizard();
begin
  WizardForm.LicenseAcceptedRadio.Checked := True;
end;

procedure ModifyMayaEnvFile(maya_version : String; action : String);
var
  ResultCode: Integer;
begin
  if Exec(ExpandConstant('{app}\MayaEnvModifier.exe'), 
      ExpandConstant(action + ' ' + '"{app}"'), '', SW_SHOW, ewWaitUntilTerminated, ResultCode)
  then 
  begin
    if not (ResultCode = 0) then
      MsgBox('Internal Setup Error: Maya.env file has not been modified! ' + maya_version, mbInformation, MB_OK);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    ModifyMayaEnvFile('2023', '-add');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    ModifyMayaEnvFile('2023', '-remove');
  end
end;