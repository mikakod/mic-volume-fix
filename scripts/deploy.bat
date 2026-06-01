@echo off
setlocal EnableExtensions

REM Copy Qt runtime DLLs next to MicVolumeFix.exe (fixes "Qt6Widgets.dll not found").
cd /d "%~dp0.."

if exist "%~dp0qt-path.bat" call "%~dp0qt-path.bat"
if "%CMAKE_PREFIX_PATH%"=="" (
  call "%~dp0find-qt.bat"
  if not errorlevel 1 set "CMAKE_PREFIX_PATH=%FOUND_QT_PREFIX%"
)
if "%CMAKE_PREFIX_PATH%"=="" (
  if exist "C:\Qt\6.11.1\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake" (
    set "CMAKE_PREFIX_PATH=C:\Qt\6.11.1\msvc2022_64"
  )
)

set "EXE=%CD%/build/Release/MicVolumeFix.exe"
if not exist "%EXE%" set "EXE=%CD%/build/MicVolumeFix.exe"
if not exist "%EXE%" (
  echo ERROR: MicVolumeFix.exe not found. Run scripts/build.bat first.
  pause
  exit /b 1
)

if "%CMAKE_PREFIX_PATH%"=="" (
  echo ERROR: Qt prefix unknown. Edit scripts/qt-path.bat
  pause
  exit /b 1
)

set "WINDEPLOYQT=%CMAKE_PREFIX_PATH%/bin/windeployqt.exe"
if not exist "%WINDEPLOYQT%" (
  echo ERROR: windeployqt not found:
  echo   %WINDEPLOYQT%
  pause
  exit /b 1
)

echo Deploying Qt runtimes for:
echo   %EXE%
echo Qt prefix:
echo   %CMAKE_PREFIX_PATH%
echo.

set "PATH=%CMAKE_PREFIX_PATH%/bin;%PATH%"
"%WINDEPLOYQT%" --release --no-translations "%EXE%"
if errorlevel 1 (
  echo.
  echo windeployqt FAILED.
  pause
  exit /b 1
)

echo.
echo Done. Run MicVolumeFix.exe from:
echo   %EXE%
if /i not "%~1"=="nopause" pause
exit /b 0
