@echo off
setlocal

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug
set BUILD_DIR=build

echo ================================================
echo   Patika C Library - Quick Build
echo   Build Type: %BUILD_TYPE%
echo ================================================

:: Clean build directory
if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
mkdir %BUILD_DIR%

:: Configure
cmake -B %BUILD_DIR% ^
      -G "Visual Studio 17 2022" ^
      -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
      -DPATIKA_BUILD_TESTS=ON ^
      -DPATIKA_BUILD_EXAMPLES=ON

if %ERRORLEVEL% neq 0 goto :error

:: Build
cmake --build %BUILD_DIR% --config %BUILD_TYPE% -j8

if %ERRORLEVEL% neq 0 goto :error

echo.
echo Build complete!
echo   Library: %BUILD_DIR%\%BUILD_TYPE%\patika_core.dll
goto :end

:error
echo Build failed!
exit /b 1

:end
endlocal
