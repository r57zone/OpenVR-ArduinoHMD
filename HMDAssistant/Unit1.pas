unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, Menus, ShellAPI, IniFiles, XPMan;

type
  TDISPLAYCONFIG_PATH_INFO = record
  end;
  PDISPLAYCONFIG_PATH_INFO = ^TDISPLAYCONFIG_PATH_INFO;

  TDISPLAYCONFIG_MODE_INFO = record
  end;
  PDISPLAYCONFIG_MODE_INFO = ^TDISPLAYCONFIG_MODE_INFO;

type
  TMain = class(TForm)
    PopupMenu1: TPopupMenu;
    CloseBtn: TMenuItem;
    N2: TMenuItem;
    N3: TMenuItem;
    RunStopBtn: TMenuItem;
    SetupBtn: TMenuItem;
    EditConfigBtn: TMenuItem;
    ResolutionsBtn: TMenuItem;
    MaxResBtn: TMenuItem;
    MiddleResBtn: TMenuItem;
    LowResBtn: TMenuItem;
    N5: TMenuItem;
    N0x01: TMenuItem;
    XPManifest1: TXPManifest;
    ModesDisplayBtn: TMenuItem;
    ExtendedBtn: TMenuItem;
    CloneBtn: TMenuItem;
    AboutBtn: TMenuItem;
    N4: TMenuItem;
    procedure FormCreate(Sender: TObject);
    procedure CloseBtnClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure RunStopBtnClick(Sender: TObject);
    procedure MiddleResBtnClick(Sender: TObject);
    procedure MaxResBtnClick(Sender: TObject);
    procedure LowResBtnClick(Sender: TObject);
    procedure EditConfigBtnClick(Sender: TObject);
    procedure CloneBtnClick(Sender: TObject);
    procedure ExtendedBtnClick(Sender: TObject);
    procedure AboutBtnClick(Sender: TObject);
  private
    procedure DefaultHandler(var Message); override;
    procedure SetResolution(Res: string);
  protected
    procedure IconMouse(var Msg: TMessage); message WM_USER + 1;
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Main: TMain;
  WM_TASKBARCREATED: Cardinal;
  EnabledIcon: boolean;
  IconFull: TIcon;
  ConfigPath, EditorPath: string;
  HMDMonitor: integer;
  MaxResolution, MiddleResolution, LowResolution: string;
  ActivateExtendedMode, SetMaxResolution, CloneModeAfterTurnOff, CloseSteamVRAfterTurnOff: boolean;

  IDS_RUN, IDS_STOP, IDS_CHANGE_RES, ID_ABOUT_TITLE, ID_LAST_UPDATE: string;

implementation

{$R *.dfm}

const
SDC_TOPOLOGY_CLONE = $00000002;
SDC_TOPOLOGY_EXTEND = $00000004;
SDC_APPLY = $00000080;
function SetDisplayConfig(numPathArrayElements: integer; pathArray: PDISPLAYCONFIG_PATH_INFO; numModeInfoArrayElements: integer; modeInfoArray: PDISPLAYCONFIG_MODE_INFO; flags: integer): longint; stdcall; external 'user32.dll'; // taken from https://github.com/CMCHTPC/WindowsAPI/blob/master/Units/Win32.WinUser.pas

procedure Tray(ActInd: integer);  //1 - Add, 2 - Update, 3 - Remove
var
  NIM: TNotifyIconData;
begin
  with NIM do begin
    cbSize:=SizeOf(NIM);
    Wnd:=Main.Handle;
    uId:=1;
    uFlags:=NIF_MESSAGE or NIF_ICON or NIF_TIP;
    
    if EnabledIcon = false then
      hIcon:=SendMessage(Application.Handle, WM_GETICON, ICON_SMALL2, 0)
    else
      hIcon:=IconFull.Handle;

    uCallBackMessage:=WM_USER + 1;
    StrCopy(szTip, PChar(Application.Title));
  end;
  case ActInd of
    1: Shell_NotifyIcon(NIM_ADD, @NIM);
    2: Shell_NotifyIcon(NIM_MODIFY, @NIM);
    3: Shell_NotifyIcon(NIM_DELETE, @NIM);
  end;
end;

procedure TMain.DefaultHandler(var Message);
begin
  if TMessage(Message).Msg = WM_TASKBARCREATED then
    Tray(1);
  inherited;
end;

function GetLocaleInformation(Flag: Integer): string;
var
  pcLCA: array [0..20] of Char;
begin
  if GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, Flag, pcLCA, 19)<=0 then
    pcLCA[0]:=#0;
  Result:=pcLCA;
end;

procedure TMain.FormCreate(Sender: TObject);
var
  Ini: TIniFile;
begin
  Ini:=TIniFile.Create(ExtractFilePath(ParamStr(0)) + 'Config.ini');
  ConfigPath:=Ini.ReadString('SteamVR', 'ConfigPath', '');
  EditorPath:=Ini.ReadString('Main', 'EditorPath', '');
  if not FileExists(ConfigPath) then ShowMessage('Конфигурационный файл SteamVR не найден');
  N0x01.Caption:=Ini.ReadString('SteamVR', 'Resolution', '0x0');
  Application.Title:='HMD assistant - ' + N0x01.Caption;
  HMDMonitor:=Ini.ReadInteger('Main', 'HMDMonitor', 0);
  MaxResolution:=Ini.ReadString('SteamVR', 'MaxResolution', '2560x1440');
  MiddleResolution:=Ini.ReadString('SteamVR', 'MiddleResolution', '1920x1080');
  LowResolution:=Ini.ReadString('SteamVR', 'LowResolution', '1600x900');
  MaxResBtn.Caption:=MaxResolution;
  MiddleResBtn.Caption:=MiddleResolution;
  LowResBtn.Caption:=LowResolution;
  SetMaxResolution:=Ini.ReadBool('Main', 'SetMaxResolution', false);
  ActivateExtendedMode:=Ini.ReadBool('Main', 'ActivateExtendedMode', false);
  CloneModeAfterTurnOff:=Ini.ReadBool('Main', 'CloneModeAfterTurnOff', false);
  CloseSteamVRAfterTurnOff:=Ini.ReadBool('Main', 'CloseSteamVRAfterTurnOff', true);
  Ini.Free;
  WM_TASKBARCREATED:=RegisterWindowMessage('TaskbarCreated');
  
  IconFull:=TIcon.Create;
  IconFull.LoadFromFile('Enabled.ico');
  EnabledIcon:=false;

  Tray(1);
  SetWindowLong(Application.Handle, GWL_EXSTYLE, GetWindowLong(Application.Handle, GWL_EXSTYLE) or WS_EX_TOOLWINDOW);

  if GetLocaleInformation(LOCALE_SENGLANGUAGE) = 'Russian' then begin
    IDS_RUN:='Включить';
    IDS_STOP:='Выключить';
    IDS_CHANGE_RES:='Разрешение изменено на';
    ID_ABOUT_TITLE:='О программе...';
    ID_LAST_UPDATE:='Последнее обновление:';
  end else begin
    IDS_RUN:='Turn on';
    IDS_STOP:='Turn off';
    RunStopBtn.Caption:=IDS_RUN;
    ResolutionsBtn.Caption:='Resolutions';
    SetupBtn.Caption:='Options';
    ModesDisplayBtn.Caption:='Display modes';
    ExtendedBtn.Caption:='Extended';
    CloneBtn.Caption:='Clone';
    EditConfigBtn.Caption:='Edit';
    ID_ABOUT_TITLE:='About...';
    AboutBtn.Caption:=ID_ABOUT_TITLE;
    ID_LAST_UPDATE:='Last update:';
    CloseBtn.Caption:='Exit';
    IDS_CHANGE_RES:='Resolution changed to';
  end;
end;

procedure TMain.IconMouse(var Msg: TMessage);
begin
  case Msg.LParam of
    WM_LBUTTONDBLCLK: RunStopBtn.Click;
    WM_LBUTTONDOWN: begin
      PostMessage(Handle, WM_LBUTTONDOWN, MK_LBUTTON, 0);
      PostMessage(Handle, WM_LBUTTONUP, MK_LBUTTON, 0);
    end;
    WM_RBUTTONDOWN:
    begin
      SetForegroundWindow(Handle);
      PopupMenu1.Popup(Mouse.CursorPos.X, Mouse.CursorPos.Y);
    end;
  end;
end;

procedure TMain.CloseBtnClick(Sender: TObject);
begin
  Close;
end;

procedure TMain.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Tray(3);
  IconFull.Free;
end;

procedure TMain.RunStopBtnClick(Sender: TObject);
begin
  if EnabledIcon = false then begin
    EnabledIcon:=true;
    RunStopBtn.Caption:=IDS_STOP;
    ShellExecute(Handle, 'open', PChar(ExtractFilePath(ParamStr(0)) + 'MultiMonitorTool.exe'), PChar('/enable \\.\DISPLAY' + IntToStr(HMDMonitor)), nil, SW_HIDE);
    if ActivateExtendedMode then
      ExtendedBtn.Click;
    if SetMaxResolution then
      ShellExecute(Handle, 'open', PChar(ExtractFilePath(ParamStr(0)) + 'MultiMonitorTool.exe'), PChar('/setmax  \\.\DISPLAY' + IntToStr(HMDMonitor)), nil, SW_HIDE);
  end else begin
    EnabledIcon:=false;
    RunStopBtn.Caption:=IDS_RUN;
    if CloneModeAfterTurnOff then
      CloneBtn.Click;
    ShellExecute(Handle, 'open', PChar(ExtractFilePath(ParamStr(0)) + 'MultiMonitorTool.exe'), PChar('/disable \\.\DISPLAY' + IntToStr(HMDMonitor)), nil, SW_HIDE);
    if CloseSteamVRAfterTurnOff then begin
      WinExec('taskkill /f /im vrserver.exe', SW_HIDE);
      WinExec('taskkill /f /im vrcompositor.exe', SW_HIDE);
      WinExec('taskkill /f /im vrdashboard.exe', SW_HIDE);
      WinExec('taskkill /f /im vrwebhelper.exe', SW_HIDE);
      WinExec('taskkill /f /im vrwebhelper.exe', SW_HIDE);
      WinExec('taskkill /f /im vrwebhelper.exe', SW_HIDE);
      WinExec('taskkill /f /im vrmonitor.exe', SW_HIDE);
    end;
  end;
  Tray(2);
end;

procedure TMain.MaxResBtnClick(Sender: TObject);
begin
  SetResolution(MaxResolution);
end;

procedure TMain.MiddleResBtnClick(Sender: TObject);
begin
  SetResolution(MiddleResolution);
end;

procedure TMain.LowResBtnClick(Sender: TObject);
begin
  SetResolution(LowResolution);
end;

procedure TMain.EditConfigBtnClick(Sender: TObject);
begin
  if (EditorPath = '') or (FileExists(EditorPath) = false) then
    ShellExecute(Handle, 'open', PChar(ConfigPath), nil, nil, SW_SHOWNORMAL)
  else
    ShellExecute(Handle, 'open', PChar(EditorPath), PChar(ConfigPath), nil, SW_SHOWNORMAL);
end;

procedure TMain.SetResolution(Res: string);
var
  i: integer; ConfigFile: TStringList; ResWidth, ResHeight: integer;
  Ini: TIniFile;
begin
  ResWidth:=StrToIntDef(Copy(Res, 1, Pos('x', Res) - 1), 0);
  ResHeight:=StrToIntDef(Copy(Res, Pos('x', Res) + 1, Length(Res)), 0);
  N0x01.Caption:=Res;
  Application.Title:='HMD assistant - ' + Res;
  Tray(2);
  Ini:=TIniFile.Create(ExtractFilePath(ParamStr(0)) + 'Config.ini');
  Ini.WriteString('SteamVR', 'Resolution', Res);
  Ini.Free;
  if ConfigPath = '' then Exit;
  ConfigFile:=TStringList.Create;
  ConfigFile.LoadFromFile(ConfigPath);
  for i:=0 to ConfigFile.Count - 1 do begin
    if Pos('renderWidth', ConfigFile.Strings[i]) > 0 then
      ConfigFile.Strings[i]:=Copy(ConfigFile.Strings[i], 1, Pos('renderWidth', ConfigFile.Strings[i]) + 11) + ' : ' + IntToStr(ResWidth) + ',';
    if Pos('renderHeight', ConfigFile.Strings[i]) > 0 then
      ConfigFile.Strings[i]:=Copy(ConfigFile.Strings[i], 1, Pos('renderHeight', ConfigFile.Strings[i]) + 12) + ' : ' + IntToStr(ResHeight) + ',';
  end;
  ConfigFile.SaveToFile(ConfigPath);
  ConfigFile.Free;
  Application.MessageBox(PChar(IDS_CHANGE_RES + ' "' + Res + '".'), PChar(Caption), MB_ICONINFORMATION);
end;

procedure TMain.CloneBtnClick(Sender: TObject);
begin
  SetDisplayConfig(0, nil, 0, nil, SDC_TOPOLOGY_CLONE or SDC_APPLY);
end;

procedure TMain.ExtendedBtnClick(Sender: TObject);
begin
  SetDisplayConfig(0, nil, 0, nil, SDC_TOPOLOGY_EXTEND or SDC_APPLY);
end;

procedure TMain.AboutBtnClick(Sender: TObject);
begin
  Application.MessageBox(PChar(Main.Caption + ' 1.2' + #13#10 +
  ID_LAST_UPDATE + ' 19.10.2022' + #13#10 +
  'https://r57zone.github.io' + #13#10 +
  'r57zone@gmail.com'), PChar(ID_ABOUT_TITLE), MB_ICONINFORMATION);
end;

end.
