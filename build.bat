@echo off
setlocal

set PRESET=%1
if "%PRESET%"=="" set PRESET=debug

if not exist "build\%PRESET%\build.ninja" (
  cmake --preset %PRESET%
)
cmake --build --preset %PRESET%

echo -- Build successful (%PRESET%)
