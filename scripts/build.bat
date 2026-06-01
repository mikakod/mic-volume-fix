@echo off
setlocal EnableExtensions

REM Native C++ build — CMake + MSVC + Qt6.
cd /d "%~dp0.."
if not exist "CMakeLists.txt" (
  echo ERROR: Expected CMakeLists.txt in repo root: %CD%
  pause
  exit /b 1
)

where cmake >nul 2>&1
if errorlevel 1 (
  echo ERROR: cmake not found. Install CMake and add it to PATH.
  pause
  exit /b 1
)

REM 1) Edit scripts/qt-path.bat to pin Qt (recommended).
if not exist "%~dp0qt-path.bat" if exist "%~dp0qt-path.bat.example" (
  copy /Y "%~dp0qt-path.bat.example" "%~dp0qt-path.bat" >nul
)
if exist "%~dp0qt-path.bat" call "%~dp0qt-path.bat"

REM 2) Auto-detect under C:\Qt
if "%CMAKE_PREFIX_PATH%"=="" (
  call "%~dp0find-qt.bat"
  if not errorlevel 1 set "CMAKE_PREFIX_PATH=%FOUND_QT_PREFIX%"
)

REM 3) Default Qt 6.11.1 MSVC kit
if "%CMAKE_PREFIX_PATH%"=="" (
  if exist "C:\Qt\6.11.1\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake" (
    set "CMAKE_PREFIX_PATH=C:\Qt\6.11.1\msvc2022_64"
  )
)

echo Repository: %CD%

if "%CMAKE_PREFIX_PATH%"=="" goto :needqt
if not exist "%CMAKE_PREFIX_PATH%\lib\cmake\Qt6\Qt6Config.cmake" (
  echo ERROR: Qt6Config.cmake missing under:
  echo   %CMAKE_PREFIX_PATH%
  goto :needqt
)

echo %CMAKE_PREFIX_PATH% | findstr /i mingw >nul
if not errorlevel 1 (
  echo.
  echo ERROR: CMAKE_PREFIX_PATH points at MinGW Qt:
  echo   %CMAKE_PREFIX_PATH%
  echo Install the MSVC 2022 64-bit Qt kit, or set CMAKE_PREFIX_PATH to msvc2022_64.
  pause
  exit /b 1
)

echo Qt prefix: %CMAKE_PREFIX_PATH%
echo.

cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" || goto :fail
cmake --build build --config Release || goto :fail

set "OUT=%CD%/build/Release/MicVolumeFix.exe"
if not exist "%OUT%" set "OUT=%CD%/build/MicVolumeFix.exe"
if not exist "%OUT%" (
  echo ERROR: MicVolumeFix.exe not found under build/
  goto :fail
)

echo.
echo Built: %OUT%
echo.
call "%~dp0deploy.bat" nopause || goto :fail
if /i not "%~1"=="nopause" pause
exit /b 0

:needqt
echo.
echo ERROR: MSVC Qt 6 kit not configured.
echo.
echo Edit: scripts/qt-path.bat
echo Example:
echo   set "CMAKE_PREFIX_PATH=C:\Qt\6.11.1\msvc2022_64"
echo.
call "%~dp0find-qt.bat" 2>nul
if /i "%FOUND_QT_KIND%"=="mingw" (
  echo Detected MinGW Qt: %FOUND_QT_PREFIX%
  echo Install MSVC 2022 64-bit Qt via Qt Maintenance Tool instead.
) else (
  echo Install Qt 6 Widgets ^(MSVC 2022 64-bit^) and set CMAKE_PREFIX_PATH.
)
pause
exit /b 1

:fail
echo.
echo Build FAILED.
pause
exit /b 1
