object Main: TMain
  Left = 192
  Top = 125
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
    object N2: TMenuItem
      Caption = '-'
    end
    object CloseBtn: TMenuItem
      Caption = #1042#1099#1093#1086#1076
      OnClick = CloseBtnClick
    end
  end
end
