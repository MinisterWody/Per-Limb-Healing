@echo off
setlocal
REM PerLimbHealing production build
REM Target: Fallout 4 OG/NG/AE desktop runtimes / F4SE 0.6.23 and 0.7.x
REM Build from a VS x64 Native Tools prompt with clang-cl/lld-link available.
REM No CRT, STL, Windows SDK import libraries, Address Library, or CommonLib binary are linked.
REM /W4 must complete with zero warnings for an accepted production build.

pushd "%~dp0"
if errorlevel 1 exit /b 1

if not exist build mkdir build
if not exist Built mkdir Built

clang-cl /nologo /c /O2 /GS- /GR- /EHs-c- /Zl /W4 /Fo:build\Plugin.obj src\Plugin.cpp
if errorlevel 1 goto :fail

lld-link /dll /noentry /nodefaultlib /opt:ref /opt:icf /timestamp:0 /machine:x64 /out:Built\PerLimbHealing.dll build\Plugin.obj
if errorlevel 1 goto :fail

popd
exit /b 0

:fail
set "PLH_BUILD_RC=%ERRORLEVEL%"
popd
exit /b %PLH_BUILD_RC%
