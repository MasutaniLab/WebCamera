@echo ���̃X�N���v�g�����s����ɂ́Ccmake�����msbuild�̑o����PATH�ɓ����Ă���K�v������܂��D
@echo cmake�̃p�X�́CC:\Program Files (x86)\CMake 2.8\bin
@echo msbuild�̃p�X�́CC:\Windows\Microsoft.NET\Framework\v4.0.30319�Ƃ��ď��������܂��D

@timeout 5

@call :BUILDW WebCamera


@timeout 10
@GOTO :EOF

:BUILDW
@if NOT EXIST build mkdir build
@cd build
@cmake ../
@msbuild %1.sln /p:Configuration=Release /p:Platform=WIN32
@if NOT EXIST components mkdir components
@copy .\src\Release\%1Comp.exe .\components\.
@copy ..\rtc.conf .\components\.
@exit /b