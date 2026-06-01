@echo off
setlocal EnableExtensions

REM Portable folder: MicVolumeFix.exe + Qt DLLs (ready to copy or zip).
cd /d "%~dp0.."

set "SRC=%CD%/build/Release"
if not exist "%SRC%/MicVolumeFix.exe" (
  echo ERROR: %SRC%/MicVolumeFix.exe not found.
  echo Run scripts/build.bat first.
  pause
  exit /b 1
)

if not exist "%SRC%/Qt6Core.dll" (
  echo Qt DLLs missing. Running deploy...
  call "%~dp0deploy.bat" nopause
  if errorlevel 1 exit /b 1
)

set "DEST=%CD%/dist/MicVolumeFix"
if exist "%DEST%" rmdir /s /q "%DEST%"
mkdir "%DEST%" 2>nul

echo Copying portable app to:
echo   %DEST%
echo.

xcopy /E /I /Y "%SRC%\*" "%DEST%\" >nul
if errorlevel 1 (
  echo ERROR: copy failed.
  pause
  exit /b 1
)

echo Done.
echo.
echo Run:  dist\MicVolumeFix\MicVolumeFix.exe
echo Zip:  dist\MicVolumeFix\  (whole folder)
if /i not "%~1"=="nopause" pause
exit /b 0
