@echo off

set SCRIPT_PATH=%~dp0

:READ_ARGS
REM For the ! variable notation
setlocal EnableDelayedExpansion
REM For shifting, which the command line argument parsing needs
setlocal EnableExtensions

set PROJECT_NAME=%2
if "!PROJECT_NAME!" == "" (
  echo No project name specified!
  exit 1
)

goto GETOPTS

:HELP
echo Usage: build-windows.bat [-hdurcqqv] [project-name]
echo  -h  Show this information
echo  -d  Faster builds that have debug symbols, and enable warnings
echo  -r  Run the executable after compilation
echo  -c  Remove the temp\{debug,release} directory, ie. full recompile
echo  -q  Suppress this script's informational prints
echo  -qq Suppress all prints, complete silence
echo.
echo Examples:
echo  Build a release build:                    build-windows.bat
echo  Build a release build, full recompile:    build-windows.bat -c
echo  Build a debug build and run:              build-windows.bat -d -r
echo  Build in debug, run, don't print at all:  build-windows.bat -drqq
exit /B

:GETOPTS
 set ARG=%1
 if NOT "x!ARG!" == "x!ARG:h=!" goto HELP
 if NOT "x!ARG!" == "x!ARG:d=!" set BUILD_DEBUG=1 & shift
 if NOT "x!ARG!" == "x!ARG:r=!" set RUN_AFTER_BUILD=1 & shift
 if NOT "x!ARG!" == "x!ARG:c=!" set BUILD_ALL=1 & shift
 if NOT "x!ARG!" == "x!ARG:qq=!" set QUIET=1 & set REALLY_QUIET=1 & shift
 if NOT "x!ARG!" == "x!ARG:v=!" set VERBOSE=1 & shift
 shift
if not "%1" == "" goto GETOPTS

:BUILD
IF DEFINED BUILD_ALL (
if exist .bin rmdir /s /q .bin
if exist .lib rmdir /s /q .lib
if exist .obj rmdir /s /q .obj
)

if not exist .bin mkdir .bin
if not exist .lib mkdir .lib
if not exist .obj mkdir .obj

set CC=emcc
set AR=emar

set COMPILE_OPTIONS=-w -g -Os -Wall -Wextra
set C_COMPILE_OPTIONS=-std=gnu99 %COMPILE_OPTIONS%
set SYSTEM_LIBS=

set RAYLIB_SRC=%SCRIPT_PATH%external\raylib\src
set RAYGUI_SRC=%SCRIPT_PATH%external\raygui\src
set RAYLIB_C_FILES="!RAYLIB_SRC!\rcore.c" "!RAYLIB_SRC!\rshapes.c" "!RAYLIB_SRC!\rtextures.c" "!RAYLIB_SRC!\rtext.c" "!RAYLIB_SRC!\rmodels.c" "!RAYLIB_SRC!\utils.c" "!RAYLIB_SRC!\raudio.c"
set RAYLIB_INCLUDES=-I%RAYLIB_SRC% -I%RAYLIB_SRC%\external\glfw\include
set "RAYLIB_DEFINES=-D_DEFAULT_SOURCE -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2"
set RAYGUI_INCLUDES=-I%SCRIPT_PATH%external\raygui\src

for %%s in (!RAYLIB_C_FILES!) do (
call :compile_2obj %%s, .obj\%%~ns
SET RAYLIB_OBJS=!RAYLIB_OBJS! %SCRIPT_PATH%.obj\%%~ns.o )


for /R "%SCRIPT_PATH%engine/src" %%f in (*.c) do (
SET "ENGINE_SRC=%%f !ENGINE_SRC!" )

set ENGINE_INCLUDES=-I%SCRIPT_PATH%engine/src

for %%s in (!ENGINE_SRC!) do (
call :compile_2obj %%s, .obj\%%~ns
SET ENGINE_OBJS=!ENGINE_OBJS! %SCRIPT_PATH%.obj\%%~ns.o )

for /R "%SCRIPT_PATH%external/box2c/src" %%f in (*.c) do (
SET "BOX2C_SRC=%%f !BOX2C_SRC!" )

rem set BOX2C_INCLUDES=-I%SCRIPT_PATH%external/box2c/include
rem set SIMDE_INCLUDE=-I%SCRIPT_PATH%external/box2c/extern/simde

rem for %%s in (!BOX2C_SRC!) do (
rem call :compile_2obj %%s, .obj\%%~ns
rem SET BOX2C_OBJS=!BOX2C_OBJS! %SCRIPT_PATH%.obj\%%~ns.o )

IF NOT DEFINED QUIET echo COMPILE-INFO: raylib compiled into object files in: !SCRIPT_PATH!.obj\

cmd /c %AR% rcs %SCRIPT_PATH%.lib\engine.a !RAYLIB_OBJS! !ENGINE_OBJS!

for /R "%SCRIPT_PATH%!PROJECT_NAME!\src" %%f in (*.c) do (
SET "GAME_SRC=%%f !GAME_SRC!" )

set GAME_SRC="!SCRIPT_PATH!!PROJECT_NAME!\src\!PROJECT_NAME!.c"

SET GAME_INCLUDES=-I%SCRIPT_PATH%!PROJECT_NAME!/src -I%RAYGUI_SRC%

echo Before compile

IF NOT DEFINED QUIET echo COMPILE-INFO: Compiling game code.
IF DEFINED REALLY_QUIET (
%CC% -o %SCRIPT_PATH%.bin\%PROJECT_NAME%.html^
 %GAME_SRC% -I. %GAME_INCLUDES% %ENGINE_INCLUDES% %BOX2C_INCLUDES% %SIMDE_INCLUDE% %RAYLIB_INCLUDES% -L. -L%SCRIPT_PATH%.lib^
  %SYSTEM_LIBS% %C_COMPILE_OPTIONS% %SCRIPT_PATH%.lib\engine.a -s ALLOW_MEMORY_GROWTH -s USE_GLFW=3 -s ASSERTIONS=0 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 --shell-file minishell.html -DPLATFORM_WEB^
  --preload-file data > NUL 2>&1
) ELSE (
%CC% -o %SCRIPT_PATH%.bin\%PROJECT_NAME%.html^
 %GAME_SRC% -I. %GAME_INCLUDES% %ENGINE_INCLUDES% %BOX2C_INCLUDES% %SIMDE_INCLUDE% %RAYLIB_INCLUDES% -L%SCRIPT_PATH%.lib^
  %SYSTEM_LIBS% %C_COMPILE_OPTIONS% %SCRIPT_PATH%.lib\engine.a -s ALLOW_MEMORY_GROWTH -s USE_GLFW=3 -s ASSERTIONS=0 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 --shell-file minishell.html -DPLATFORM_WEB^
  --preload-file data
)

IF NOT DEFINED REALLY_QUIET (
call :error_check .bin\%PROJECT_NAME%.html
)

IF DEFINED RUN_AFTER_BUILD (
  IF NOT DEFINED QUIET echo COMPILE-INFO: Running.
  IF DEFINED REALLY_QUIET (
    .bin\%PROJECT_NAME%.html > NUL 2>&1
  ) ELSE (
    .bin\%PROJECT_NAME%.html
  )
)

goto END
 
:compile_2obj (
  IF DEFINED REALLY_QUIET (
    %CC% -c %~1 -o %SCRIPT_PATH%%~2.o %ENGINE_INCLUDES% %BOX2C_INCLUDES% %SIMDE_INCLUDE% %RAYGUI_INCLUDES% %RAYLIB_INCLUDES% %C_COMPILE_OPTIONS% !RAYLIB_DEFINES! > NUL 2>&1
  ) ELSE (
    %CC% -c %~1 -o %SCRIPT_PATH%%~2.o %ENGINE_INCLUDES% %BOX2C_INCLUDES% %SIMDE_INCLUDE% %RAYGUI_INCLUDES% %RAYLIB_INCLUDES% %C_COMPILE_OPTIONS% !RAYLIB_DEFINES!
    call :error_check %~2.o
  )
  goto :eof
)

:error_check (
  if %ERRORLEVEL% equ 0 (
  echo %~1 : success
  ) else (
  echo Error code: %ERRORLEVEL%
  exit %ERRORLEVEL%
  )
  goto :eof
)

:END

endlocal
