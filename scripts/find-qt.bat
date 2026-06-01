@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM Locate Qt 6 under C:\Qt. Sets FOUND_QT_PREFIX (+ FOUND_QT_KIND=msvc|mingw).
REM Must use endlocal&set so callers see variables (setlocal would drop them on exit).
set "FOUND_QT_PREFIX="
set "FOUND_QT_KIND="

if not "%CMAKE_PREFIX_PATH%"=="" (
  if exist "%CMAKE_PREFIX_PATH%\lib\cmake\Qt6\Qt6Config.cmake" (
    set "FOUND_QT_PREFIX=%CMAKE_PREFIX_PATH%"
    echo %CMAKE_PREFIX_PATH% | findstr /i mingw >nul && (
      set "FOUND_QT_KIND=mingw"
    ) || (
      set "FOUND_QT_KIND=msvc"
    )
    goto :done_ok
  )
)

for /f "delims=" %%V in ('dir /b /ad /o-n "C:\Qt\6.*" 2^>nul') do (
  for %%K in (msvc2022_64 msvc2019_64) do (
    set "CAND=C:\Qt\%%V\%%K"
    if exist "!CAND!\lib\cmake\Qt6\Qt6Config.cmake" (
      set "FOUND_QT_PREFIX=!CAND!"
      set "FOUND_QT_KIND=msvc"
      goto :done_ok
    )
  )
)

for /f "delims=" %%V in ('dir /b /ad /o-n "C:\Qt\6.*" 2^>nul') do (
  for /d %%K in ("C:\Qt\%%V\msvc*_64") do (
    if exist "%%K\lib\cmake\Qt6\Qt6Config.cmake" (
      set "FOUND_QT_PREFIX=%%K"
      set "FOUND_QT_KIND=msvc"
      goto :done_ok
    )
  )
)

for /f "delims=" %%V in ('dir /b /ad /o-n "C:\Qt\6.*" 2^>nul') do (
  set "CAND=C:\Qt\%%V\mingw_64"
  if exist "!CAND!\lib\cmake\Qt6\Qt6Config.cmake" (
    set "FOUND_QT_PREFIX=!CAND!"
    set "FOUND_QT_KIND=mingw"
    goto :done_ok
  )
)

endlocal & exit /b 1

:done_ok
endlocal & set "FOUND_QT_PREFIX=%FOUND_QT_PREFIX%" & set "FOUND_QT_KIND=%FOUND_QT_KIND%" & exit /b 0
