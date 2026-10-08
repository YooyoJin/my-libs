@echo off
chcp 65001 >nul
cd /d %~dp0

if "%1"=="arm"   goto arm
if "%1"=="all"   goto all
if "%1"=="run"   goto run
goto pc

:pc
cmake -S projects/gcc -B projects/gcc/build -G "MinGW Makefiles" || exit /b 1
cmake --build projects/gcc/build || exit /b 1
echo [OK] projects/gcc/build/fanctrldemo.exe
exit /b 0

:run
call build.bat pc || exit /b 1
projects\gcc\build\fanctrldemo.exe
exit /b 0

:arm
cmake -S projects/armgcc -B projects/armgcc/build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%~dp0projects\armgcc\toolchainarmgcc.cmake" || exit /b 1
cmake --build projects/armgcc/build || exit /b 1
echo [OK] projects/armgcc/build/libfanctrl.a
exit /b 0

:all
call build.bat pc || exit /b 1
call build.bat arm || exit /b 1
exit /b 0
