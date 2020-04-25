@echo on

if NOT EXIST C:\projects\tools (
  mkdir C:\projects\tools
)
cd C:\projects\tools

::###########################################################################
:: Setup Compiler
::###########################################################################
if NOT EXIST llvm-installer.exe (
    appveyor DownloadFile http://prereleases.llvm.org/win-snapshots/LLVM-5.0.0-r306282-win32.exe -FileName llvm-installer.exe
)

START /WAIT llvm-installer.exe /S /D=C:\"projects\tools\LLVM-install"
@set PATH="C:\projects\tools\LLVM-install\bin";%PATH%
clang-cl -v

if DEFINED MINGW_PATH rename "C:\Program Files\Git\usr\bin\sh.exe" "sh-ignored.exe"
if DEFINED MINGW_PATH @set "PATH=%PATH:C:\Program Files (x86)\Git\bin=%"
if DEFINED MINGW_PATH @set "PATH=%PATH%;%MINGW_PATH%"
if DEFINED MINGW_PATH g++ -v

::###########################################################################
:: Install a recent CMake
::###########################################################################
if NOT EXIST cmake (
  appveyor DownloadFile https://cmake.org/files/v3.7/cmake-3.7.2-win64-x64.zip -FileName cmake.zip
  7z x cmake.zip -oC:\projects\tools > nul
  move C:\projects\tools\cmake-* C:\projects\tools\cmake
  rm cmake.zip
)
@set PATH=C:\projects\tools\cmake\bin;%PATH%
cmake --version

::###########################################################################
:: Install Ninja
::###########################################################################
if NOT EXIST ninja (
  appveyor DownloadFile https://github.com/ninja-build/ninja/releases/download/v1.6.0/ninja-win.zip -FileName ninja.zip
  7z x ninja.zip -oC:\projects\tools\ninja > nul
  rm ninja.zip
)
@set PATH=C:\projects\tools\ninja;%PATH%
ninja --version

@echo off
