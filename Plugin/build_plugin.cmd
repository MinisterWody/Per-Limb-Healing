@echo off
REM PerLimbHealing production build
REM Target: Fallout 4 OG/NG/AE desktop runtimes / F4SE 0.6.23 and 0.7.x
REM Build from a VS x64 Native Tools prompt with clang-cl/lld-link available.
REM No CRT, STL, Windows SDK import libraries, Address Library, or CommonLib binary are linked.
REM /W4 must complete with zero warnings for an accepted production build.
if not exist build mkdir build
clang-cl /nologo /c /O2 /GS- /GR- /EHs-c- /Zl /W4 /Fo:build\Plugin.obj src\Plugin.cpp
if errorlevel 1 exit /b 1
lld-link /dll /noentry /nodefaultlib /opt:ref /opt:icf /timestamp:0 /machine:x64 /out:build\PerLimbHealing.dll build\Plugin.obj
