@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt
set LIB=%WDK_ROOT%\lib\wnet\amd64;%WDK_ROOT%\lib\Crt\amd64
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin\amd64;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin\x64;%SystemRoot%\SysWOW64
set X64=1
cscript configure.js compiler=msvc iconv=no iso8859x=yes
nmake /nologo /f Makefile.msvc clean
nmake /nologo /f Makefile.msvc
rmdir /q /s bin
rmdir /q /s include
rmdir /q /s lib
nmake /nologo /f Makefile.msvc install
move lib\libxml2.dll bin
set PATH=%PATH%;%PROGRAMW6432%\7-Zip
7z a "LibXML-WDK 2.7.8-x64.7z" bin include lib
pause