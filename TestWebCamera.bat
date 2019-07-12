@echo off
: WebCameraのテスト

:ネーミングサービスの確認
rtls /localhost > nul
if errorlevel 1 (
  echo ネーミングサーバが見つかりません
  pause
  exit /b 1
  rem /bオプションは親を終わらせないために必須
)

:コンポーネントの起動
call ..\WebCamera\WebCamera.bat
call ..\ImageViewer\ImageViewer.bat

:コンポーネント名を変数化
set c=/localhost/WebCamera0.rtc
set v=/localhost/ImageViewer0.rtc

:時間待ち
timeout 5

:接続
rtcon %c%:CameraImage %v%:Image

:アクティベート
rtact %c% %v%

:loop
set /p ans="終了しますか？ (y/n)"
if not "%ans%"=="y" goto loop

:ディアクティベート
rtdeact %c% %v%

:終了（rtexitは，引数を一つずつ）
rtexit %c%
rtexit %v%
