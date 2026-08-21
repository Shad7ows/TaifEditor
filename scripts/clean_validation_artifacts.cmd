@echo off
setlocal EnableExtensions

set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
cd /d "%ROOT%"

for %%D in (tests\lexer tests\parser tests\semantic tests\analysis tests\controllers tests\ui) do (
    if exist "%%D\release" rmdir /s /q "%%D\release"
    if exist "%%D\debug" rmdir /s /q "%%D\debug"
    del /q "%%D\.qmake.stash" "%%D\Makefile" "%%D\Makefile.Debug" "%%D\Makefile.Release" "%%D\target_wrapper.bat" 2>nul
)

echo Removed generated validation artifacts. Source, documentation, and application build outputs were not changed.
exit /b 0
