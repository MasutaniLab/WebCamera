@echo off
: WebCamera�̃e�X�g

:�l�[�~���O�T�[�r�X�̊m�F
rtls /localhost > nul
if errorlevel 1 (
  echo �l�[�~���O�T�[�o��������܂���
  pause
  exit /b 1
  rem /b�I�v�V�����͐e���I��点�Ȃ����߂ɕK�{
)

:�R���|�[�l���g�̋N��
call ..\WebCamera\WebCamera.bat
call ..\ImageViewer\ImageViewer.bat

:�R���|�[�l���g����ϐ���
set c=/localhost/WebCamera0.rtc
set v=/localhost/ImageViewer0.rtc

:���ԑ҂�
timeout 5

:�ڑ�
rtcon %c%:CameraImage %v%:Image

:�A�N�e�B�x�[�g
rtact %c% %v%

:loop
set /p ans="�I�����܂����H (y/n)"
if not "%ans%"=="y" goto loop

:�f�B�A�N�e�B�x�[�g
rtdeact %c% %v%

:�I���irtexit�́C����������j
rtexit %c%
rtexit %v%
