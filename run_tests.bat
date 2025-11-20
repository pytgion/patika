@echo off
REM run_tests.bat - Windows test runner for Patika library
setlocal enabledelayedexpansion

set BUILD_DIR=build
set VERBOSE=0
set CLEAN=0
set SPECIFIC_TEST=

REM Parse arguments
:parse_args
if "%~1"=="" goto end_parse
if /I "%~1"=="-h" goto show_help
if /I "%~1"=="--help" goto show_help
if /I "%~1"=="-v" (
    set VERBOSE=1
    shift
    goto parse_args
)
if /I "%~1"=="-c" (
    set CLEAN=1
    shift
    goto parse_args
)
if /I "%~1"=="-t" (
    set SPECIFIC_TEST=%~2
    shift
    shift
    goto parse_args
)
shift
goto parse_args

:show_help
echo Patika Test Runner (Windows)
echo.
echo Usage: run_tests.bat [OPTIONS]
echo.
echo OPTIONS:
echo     -h, --help          Show this help message
echo     -v                  Verbose test output
echo     -c                  Clean build directory before building
echo     -t NAME             Run specific test
echo.
echo EXAMPLES:
echo     run_tests.bat                      # Run all tests
echo     run_tests.bat -v                   # Verbose output
echo     run_tests.bat -t test_agent_pool   # Run specific test
echo     run_tests.bat -c                   # Clean rebuild and test
echo.
exit /b 0

:end_parse

REM Clean if requested
if %CLEAN%==1 (
    echo [CLEAN] Removing build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure
echo [CONFIG] Configuring build...
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

REM Build
echo [BUILD] Building...
cmake --build . --config Release
if errorlevel 1 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo [SUCCESS] Build successful!
echo.

REM Run tests
if not "%SPECIFIC_TEST%"=="" (
    echo [TEST] Running %SPECIFIC_TEST%...
    Release\%SPECIFIC_TEST%.exe
) else (
    echo [TEST] Running all tests...
    if %VERBOSE%==1 (
        ctest -C Release -V
    ) else (
        ctest -C Release --output-on-failure
    )
)

if errorlevel 1 (
    echo.
    echo [FAILED] Some tests failed!
    exit /b 1
) else (
    echo.
    echo [PASSED] All tests passed!
    exit /b 0
)
