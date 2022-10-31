object Main: TMain
  Left = 193
  Top = 126
  Width = 336
  Height = 279
  Caption = 'HMD assistant'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object PopupMenu1: TPopupMenu
    Left = 8
    Top = 8
    object RunStopBtn: TMenuItem
      Caption = #1042#1082#1083#1102#1095#1080#1090#1100
      OnClick = RunStopBtnClick
    end
    object N3: TMenuItem
      Caption = '-'
    end
    object ResolutionsBtn: TMenuItem
      Caption = #1056#1072#1079#1088#1077#1096#1077#1085#1080#1077
      object N0x01: TMenuItem
        Caption = '0x0'
        Enabled = False
      end
      object N5: TMenuItem
        Caption = '-'
      end
      object MaxResBtn: TMenuItem
        Caption = '2560x1440'
        OnClick = MaxResBtnClick
      end
      object MiddleResBtn: TMenuItem
        Caption = '1920x1080'
        OnClick = MiddleResBtnClick
      end
      object LowResBtn: TMenuItem
        Caption = '1600x900'
        OnClick = LowResBtnClick
      end
    end
    object SetupBtn: TMenuItem
      Caption = #1055#1072#1088#1072#1084#1077#1090#1088#1099
      object EditConfigBtn: TMenuItem
        Caption = #1048#1079#1084#1077#1085#1080#1090#1100
        OnClick = EditConfigBtnClick
      end
    end
    object ModesDisplayBtn: TMenuItem
      Caption = #1056#1077#1078#1080#1084#1099' '#1101#1082#1088#1072#1085#1086#1074
      object ExtendedBtn: TMenuItem
        Caption = #1056#1072#1089#1096#1080#1088#1077#1085#1085#1099#1081
        OnClick = ExtendedBtnClick
      end
      object CloneBtn: TMenuItem
        Caption = #1055#1086#1074#1090#1086#1088#1103#1102#1097#1080#1081#1089#1103
        OnClick = CloneBtnClick
      end
    end
    object N2: TMenuItem
      Caption = '-'
    end
    object AboutBtn: TMenuItem
      Caption = #1054' '#1087#1088#1086#1075#1088#1072#1084#1084#1077'...'
      OnClick = AboutBtnClick
    end
    object N4: TMenuItem
      Caption = '-'
    end
    object CloseBtn: TMenuItem
      Caption = #1042#1099#1093#1086#1076
      OnClick = CloseBtnClick
    end
  end
  object XPManifest1: TXPManifest
    Left = 40
    Top = 8
  end
end
