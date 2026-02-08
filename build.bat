@echo off
setlocal enabledelayedexpansion

call "%VCVARS_PATH%" x64

REM Start timer
for /f %%i in ('powershell -command "[Environment]::TickCount"') do set START_TIME=%%i

set BUILD_DIR=build
set EDITOR_MODE_FLAG=0
set DEMO_FLAG=0
set PACKAGE_FILES=0
set DEBUG_SET=0

REM We use /Zi and /DEBUG to emit a .pdb file that we can use for crash analysis. We do NOT ship the pdb, and our 
REM .exe should remain clean and optimized even with these flags.
set COMPILER_FLAGS=/MT /O2 /Zi /GL /Ob2 /Oy
set LINKER_FLAGS=/DEBUG
set LIB_PATH=libraries/x64
set OUTPUT_NAME=svo.exe

for %%a in (%*) do (
    if "%%a"=="Debug" (
    	set COMPILER_FLAGS=/Od /Ob0 /Zi /MTd /D "_DEBUG"
    	set LIB_PATH=libraries/debug/x64
    	set DEBUG_SET=1
    	echo Using DEBUG build
    ) else if "%%a"=="/editor" (
        set EDITOR_MODE_FLAG=1
    ) else if "%%a"=="/package" (
        set PACKAGE_FILES=1
    ) else if "%%a"=="/demo" (
        set DEMO_FLAG=1
    )
)

if %EDITOR_MODE_FLAG%==1 (
    set COMPILER_FLAGS=%COMPILER_FLAGS% /D EDITOR_MODE
)

if %DEMO_FLAG%==1 (
    set COMPILER_FLAGS=%COMPILER_FLAGS% /D DEMO
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cl %COMPILER_FLAGS% /I "include" /Fd:%BUILD_DIR%/ /Fo:%BUILD_DIR%/ /Fe:"%OUTPUT_NAME%" ^
src/win32_main.cpp ^
/link /PDB:%BUILD_DIR%/ /LIBPATH:"%LIB_PATH%" Shell32.lib User32.lib winmm.lib d3d11.lib d3d12.lib d3dcompiler.lib dxgi.lib dxguid.lib Comdlg32.lib ^
xinput.lib dbghelp.lib /subsystem:windows

@echo off

REM Check for compile errors
if errorlevel 1 (
	powershell -Command "Write-Host 'Compilation failed!' -ForegroundColor Red"
	exit /b 1
)

if %DEBUG_SET% == 1 (
    powershell -Command "Write-Host 'Debug build complete!' -ForegroundColor Green"
) else (
	powershell -Command "Write-Host 'Release build complete!' -ForegroundColor Green"
)

REM End timer
for /f %%i in ('powershell -command "[Environment]::TickCount"') do set END_TIME=%%i

set /a ELAPSED=!END_TIME! - !START_TIME!
echo Build time: !ELAPSED! ms

endlocal