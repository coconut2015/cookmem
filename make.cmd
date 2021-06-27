@echo off

SETLOCAL

set BUILD=%1

if "%BUILD%" == "" (
	set BUILD=debug
)

set BUILD_DOC=
set PROFILER=

if "%BUILD%" == "release" goto RELEASE_1
if "%BUILD%" == "debug" goto DEBUG_1
if "%BUILD%" == "profiler" goto PROFILER_1

echo "Unknown build option: %BUILD%"
goto END

:RELEASE_1
	goto END_1

:DEBUG_1
	goto END_1

:PROFILER_1
	set PROFILER="-DVCPROFILER=1"
	goto END_1

:END_1

if not exist "%BUILD%\" (
	md %BUILD%
)

cd %BUILD%

cmake %PROFILER% %BUILD_DOC% ..

if %ERRORLEVEL% GEQ 1 goto END

if "%BUILD%" == "release" goto RELEASE_2
if "%BUILD%" == "debug" goto DEBUG_2
if "%BUILD%" == "profiler" goto PROFILER_2

:RELEASE_2
	cmake --build . --config Release
	if %ERRORLEVEL% EQU 0 ctest -C Release
	goto END_2

:DEBUG_2
	cmake --build . --config Debug
	if %ERRORLEVEL% EQU 0 ctest -C Debug
	goto END_2

:PROFILER_2
	cmake --build . --config Release
	if %ERRORLEVEL% EQU 0 ctest -C Release
	goto END_2

:END_2

ENDLOCAL

:END
