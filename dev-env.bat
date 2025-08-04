@echo off
:: Enhanced development environment script for CppGraphIndex
:: This script sets up the Visual Studio environment and provides convenient commands

if "%1"=="" goto help

:: Set up Visual Studio environment via Conan
call "%~dp0\conanbuild.bat" >nul 2>&1

set COMMAND=%1
if "%COMMAND%"=="build" goto build
if "%COMMAND%"=="test" goto test
if "%COMMAND%"=="format" goto format
if "%COMMAND%"=="lint" goto lint
if "%COMMAND%"=="clean" goto clean
if "%COMMAND%"=="shell" goto shell
if "%COMMAND%"=="python" goto python
goto unknown

:help
echo [DEV-ENV] Setting up Visual Studio 2022 environment...
call "%~dp0\conanbuild.bat"
echo.
echo [DEV-ENV] Environment ready! Available commands:
echo.
echo Usage: dev-env.bat [command]
echo.
echo Commands:
echo   build      - Build the project (xmake)
echo   test       - Run tests (xmake test)
echo   format     - Format code (xmake run format)
echo   lint       - Lint code (xmake run lint)
echo   clean      - Clean build artifacts (xmake clean)
echo   shell      - Start an interactive shell with environment
echo   python     - Run Python build script with args
echo.
echo Examples:
echo   dev-env.bat build
echo   dev-env.bat test
echo   dev-env.bat python tools/build.py full
echo   dev-env.bat shell
echo.
goto end

:build
echo [DEV-ENV] Running xmake build...
shift
xmake %*
goto end

:test
echo [DEV-ENV] Running tests...
shift
xmake test %*
goto end

:format
echo [DEV-ENV] Formatting code...
shift
xmake run format %*
goto end

:lint
echo [DEV-ENV] Linting code...
shift
xmake run lint %*
goto end

:clean
echo [DEV-ENV] Cleaning...
shift
xmake clean %*
goto end

:shell
echo [DEV-ENV] Starting interactive shell with VS environment...
echo [INFO] You can now run any commands with the VS environment active.
echo [INFO] Type 'exit' to leave this environment.
cmd /k
goto end

:python
echo [DEV-ENV] Running Python script...
shift
python %*
goto end

:unknown
echo [ERROR] Unknown command: %COMMAND%
echo Run 'dev-env.bat' without arguments to see available commands.
goto end

:end