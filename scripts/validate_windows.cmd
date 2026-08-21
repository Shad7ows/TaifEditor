@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Reproducible TaifEditor validation gate for Windows + Qt 6 + MSVC 2022.
rem Optional --ci verifies the checkout is clean before and after the build.
set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
set "CI_MODE=0"
if /I "%~1"=="--ci" set "CI_MODE=1"

if not defined VSDEVCMD set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not defined QT_ROOT set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"

if not exist "%VSDEVCMD%" (
    echo ERROR: Visual Studio developer environment was not found: "%VSDEVCMD%"
    echo Set VSDEVCMD to VsDevCmd.bat and rerun.
    exit /b 2
)
if not exist "%QT_ROOT%\bin\qmake.exe" (
    echo ERROR: Qt MSVC kit was not found: "%QT_ROOT%"
    echo Set QT_ROOT to the desired Qt kit root and rerun.
    exit /b 2
)

call "%VSDEVCMD%" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
set "PATH=%QT_ROOT%\bin;%PATH%"
set "QT_QPA_PLATFORM_PLUGIN_PATH=%QT_ROOT%\plugins\platforms"
set "QT_QPA_PLATFORM=windows"

cd /d "%ROOT%"
if "%CI_MODE%"=="1" call :require_clean_tree "before validation"
if errorlevel 1 exit /b %errorlevel%

call :build_application
if errorlevel 1 goto :failure
call :build_and_run "tests\lexer" "lexer_tests.pro" "release\TaifLexerTests.exe"
if errorlevel 1 goto :failure
call :build_and_run "tests\parser" "parser_tests.pro" "release\TaifParserTests.exe"
if errorlevel 1 goto :failure
call :build_and_run "tests\semantic" "semantic_tests.pro" "release\TaifSemanticTests.exe"
if errorlevel 1 goto :failure
call :build_and_run "tests\analysis" "analysis_tests.pro" "release\TaifAnalysisTests.exe"
if errorlevel 1 goto :failure
call :build_and_run "tests\controllers" "controllers_tests.pro" "release\TaifControllerTests.exe"
if errorlevel 1 goto :failure
call :build_and_run "tests\ui" "ui_tests.pro" "release\TaifDockableToolsTests.exe"
if errorlevel 1 goto :failure

cd /d "%ROOT%"
git -c core.pager=cat diff --check
if errorlevel 1 goto :failure
if "%CI_MODE%"=="1" call :require_clean_tree "after validation"
if errorlevel 1 goto :failure

echo.
echo PASS: TaifEditor application, lexer, parser, semantic, analysis, controller, UI, and hygiene gates passed.
exit /b 0

:build_application
cd /d "%ROOT%\taif\build\analysis_validation"
qmake ..\..\Taif.pro
if errorlevel 1 exit /b %errorlevel%
nmake /NOLOGO
exit /b %errorlevel%

:build_and_run
set "TEST_DIRECTORY=%~1"
set "PROJECT_FILE=%~2"
set "TEST_EXE=%~3"
cd /d "%ROOT%\%TEST_DIRECTORY%"
qmake "%PROJECT_FILE%"
if errorlevel 1 exit /b %errorlevel%
nmake /NOLOGO
if errorlevel 1 exit /b %errorlevel%
"%TEST_EXE%" -txt
exit /b %errorlevel%

:require_clean_tree
for /f "usebackq delims=" %%S in (`git status --porcelain`) do (
    echo ERROR: Validation requires a clean checkout %~1.
    echo        %%S
    exit /b 1
)
exit /b 0

:failure
set "RESULT=%errorlevel%"
echo.
echo FAIL: TaifEditor validation stopped with exit code %RESULT%.
exit /b %RESULT%
