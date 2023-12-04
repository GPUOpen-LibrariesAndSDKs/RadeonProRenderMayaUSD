#define AppVersionString '1.0.3'

[Setup]
AppName=RPRMayaUSD_{#MayaVersionString}
AppVersion={#AppVersionString}
WizardStyle=modern
DefaultDirName="{autopf64}\RPRMayaUSD_{#MayaVersionString}"
DefaultGroupName=RPRMayaUSD_{#MayaVersionString}
ChangesEnvironment=no
OutputDir=.
OutputBaseFilename=RPRMayaUSD_{#MayaVersionString}_{#AppVersionString}_Setup
SetupIconFile=.\ico\AMD.ico
//LicenseFile=LICENSE.txt

[Files]
//Source: "..\LICENSE.txt"; DestDir: "{app}";
Source: "..\Build_RPRUsdInstall\*"; DestDir: "{app}"; Flags: recursesubdirs

[InstallDelete]
Type: filesandordirs; Name: "{app}"

[UninstallDelete]
Type: filesandordirs; Name: "{%LOCALAPPDATA}\RadeonProRender\Maya\USD\"
Type: filesandordirs; Name: "{%USERPROFILE}\Documents\Maya\RprUsd\WebMatlibCache"

[Code]
procedure InitializeWizard();
begin
  WizardForm.LicenseAcceptedRadio.Checked := True;  
end;

procedure ModifyMayaEnvFile(maya_version : String; action : String);
var
  resultCode: Integer;
begin
  if Exec(ExpandConstant('{app}\MayaEnvModifier.exe'), 
      ExpandConstant(action + ' ' + '"{app}"' + ' ' + maya_version), '', SW_SHOW, ewWaitUntilTerminated, resultCode)
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
  mayaVersion : String;
begin
  mayaVersion := ExpandConstant('{#MayaVersionString}');

  if CurStep = ssPostInstall then
  begin
    ModifyMayaEnvFile(MayaVersion, '-add');

    mayaModulePath := ExpandConstant('{commoncf64}\Autodesk Shared\modules\maya\' + mayaVersion);

    if Exec(ExpandConstant('{app}\rprUsdModModifier.exe'), 
        ExpandConstant('"{app}\rprUsd\rprUsd.mod" "{app}\rprUsd" ' + mayaVersion), '', SW_SHOW, ewWaitUntilTerminated, resultCode)
    then 
    begin
      if not (resultCode = 0) then
        MsgBox('Internal Setup Error: rprUsd.mod file has not been modified! ' + mayaVersion, mbInformation, MB_OK);
    end;
    
    if DirExists(mayaModulePath) then
      if not FileCopy(ExpandConstant('{app}\rprUsd\rprUsd.mod'), mayaModulePath + '\rprUsd.mod', false)
      then
        MsgBox('Setup Error: RprUsd.mod file could not be copied to Maya''s modules directory! ' + mayaVersion, mbInformation, MB_OK);

  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  path : String;
  mayaVersion : String;
begin
  mayaVersion := ExpandConstant('{#MayaVersionString}');
  if CurUninstallStep = usUninstall then
  begin
    path := ExpandConstant('{commoncf64}\Autodesk Shared\modules\maya\' + mayaVersion + '\rprUsd.mod');
    DeleteFile(path);

    ModifyMayaEnvFile(mayaVersion, '-remove');
  end
end;
