@echo off
REM Load RT-Thread Env into the current CMD session.
REM
REM One-time machine setup (normally no arguments are needed when RT-Thread
REM Studio is installed under <drive>:\Work\RT-ThreadStudio or
REM <drive>:\RT-ThreadStudio):
REM   powershell -NoProfile -ExecutionPolicy Bypass -File tools\env\setup_rtt_env.ps1
REM
REM Normal project use:
REM   call tools\env\rtt_env.cmd
REM   call tools\env\rtt_env.cmd scons

set "PROJECT_DIR=%CD%"
set "RTT_ENV_DIR="
set "ENV_INIT="
set "PREFERRED_RTT_EXEC_PATH=%RTT_EXEC_PATH%"
call :resolve_preferred_gcc

REM 1) Prefer a valid current-session RTT_ENV_HOME, then refresh User-level
REM machine configuration directly from HKCU when the inherited value is absent
REM or stale.
call :try_env_home "%RTT_ENV_HOME%"
if not "%RTT_ENV_DIR%"=="" goto :env_resolved

set "RTT_ENV_HOME="
call :read_user_env RTT_ENV_HOME
call :try_env_home "%RTT_ENV_HOME%"
if not "%RTT_ENV_DIR%"=="" goto :env_resolved

REM 2) An already active Env is authoritative when no valid RTT_ENV_HOME was
REM found. This preserves manually opened RT-Thread Env terminals.
call :try_env_home "%ENV_ROOT%"
if not "%RTT_ENV_DIR%"=="" goto :env_resolved

REM 3) Accept RTT_STUDIO_HOME from the current/User environment.
call :try_studio_home "%RTT_STUDIO_HOME%"
if not "%RTT_ENV_DIR%"=="" goto :env_resolved

set "RTT_STUDIO_HOME="
call :read_user_env RTT_STUDIO_HOME
call :try_studio_home "%RTT_STUDIO_HOME%"
if not "%RTT_ENV_DIR%"=="" goto :env_resolved

REM 4) Compatibility auto-discovery. Check only conventional Studio roots;
REM do not recursively scan user disks.
call :find_common_env

:env_resolved
if "%RTT_ENV_DIR%"=="" goto :env_not_found
if exist "%RTT_ENV_DIR%\tools\bin\env-init.bat" set "ENV_INIT=%RTT_ENV_DIR%\tools\bin\env-init.bat"
if "%ENV_INIT%"=="" if exist "%RTT_ENV_DIR%\env.bat" set "ENV_INIT=%RTT_ENV_DIR%\env.bat"
if "%ENV_INIT%"=="" goto :env_not_found

REM 5) Do not combine two different active Env installations.
if not "%ENV_ROOT%"=="" goto :check_loaded_env
goto :init_env

:check_loaded_env
for %%I in ("%RTT_ENV_DIR%") do set "RTT_ENV_DIR_ABS=%%~fI"
for %%I in ("%ENV_ROOT%") do set "LOADED_ENV_DIR_ABS=%%~fI"
if /i "%RTT_ENV_DIR_ABS%"=="%LOADED_ENV_DIR_ABS%" goto :env_ready

echo [rtt_env] ERROR: A different RT-Thread Env is already loaded.
echo [rtt_env] Loaded:    "%LOADED_ENV_DIR_ABS%"
echo [rtt_env] Requested: "%RTT_ENV_DIR_ABS%"
echo [rtt_env] Open a new terminal or clear the conflicting environment first.
exit /b 1

:init_env
call "%ENV_INIT%"
set "ENV_INIT_RC=%errorlevel%"
if not "%ENV_INIT_RC%"=="0" goto :env_init_failed

:env_ready
cd /d "%PROJECT_DIR%"

REM RT-Thread Env 2.x exports PKGS_ROOT. Keep a deterministic fallback for
REM older Env layouts because ART-PI sources selected package Kconfig files
REM directly from the package index.
if "%PKGS_ROOT%"=="" if not "%ENV_ROOT%"=="" set "PKGS_ROOT=%ENV_ROOT%\packages"
if "%PKGS_ROOT%"=="" goto :packages_index_not_found
if not exist "%PKGS_ROOT%\packages\Kconfig" goto :packages_index_not_found

where scons >nul 2>nul
if errorlevel 1 goto :scons_not_found

REM Restore the valid current/User compiler selected before Env initialization.
REM If neither is valid, keep Env's compiler; only fall back to PATH when Env did
REM not provide a usable arm-none-eabi-gcc.exe.
if not "%PREFERRED_RTT_EXEC_PATH%"=="" if exist "%PREFERRED_RTT_EXEC_PATH%\arm-none-eabi-gcc.exe" set "RTT_EXEC_PATH=%PREFERRED_RTT_EXEC_PATH%"
if not "%RTT_EXEC_PATH%"=="" if exist "%RTT_EXEC_PATH%\arm-none-eabi-gcc.exe" goto :compiler_ready
set "RTT_EXEC_PATH="

goto :discover_gcc

:discover_gcc
for /f "delims=" %%I in ('where arm-none-eabi-gcc.exe 2^>nul') do if not defined RTT_EXEC_PATH set "RTT_EXEC_PATH=%%~dpI"

:compiler_ready
if "%RTT_EXEC_PATH%"=="" goto :compiler_not_found
if not exist "%RTT_EXEC_PATH%\arm-none-eabi-gcc.exe" goto :compiler_not_found
if "%RTT_EXEC_PATH:~-1%"=="\" set "RTT_EXEC_PATH=%RTT_EXEC_PATH:~0,-1%"

if not "%~1"=="" goto :run_command

echo [rtt_env] OK: RT-Thread Env is ready.
echo [rtt_env] Env: "%RTT_ENV_DIR%"
echo [rtt_env] PKGS_ROOT: "%PKGS_ROOT%"
echo [rtt_env] RTT_EXEC_PATH: "%RTT_EXEC_PATH%"
exit /b 0

:run_command
call %*
exit /b %errorlevel%

:resolve_preferred_gcc
REM A valid inherited compiler wins. If it is missing/stale, refresh the latest
REM Windows User value before env-init.bat overwrites RTT_EXEC_PATH.
if not "%PREFERRED_RTT_EXEC_PATH%"=="" if exist "%PREFERRED_RTT_EXEC_PATH%\arm-none-eabi-gcc.exe" exit /b 0
set "RTT_EXEC_PATH="
call :read_user_env RTT_EXEC_PATH
if not "%RTT_EXEC_PATH%"=="" if exist "%RTT_EXEC_PATH%\arm-none-eabi-gcc.exe" set "PREFERRED_RTT_EXEC_PATH=%RTT_EXEC_PATH%"
exit /b 0

:try_env_home
if "%~1"=="" exit /b 0
if exist "%~1\tools\bin\env-init.bat" set "RTT_ENV_DIR=%~1"
if "%RTT_ENV_DIR%"=="" if exist "%~1\env.bat" set "RTT_ENV_DIR=%~1"
exit /b 0

:try_studio_home
if "%~1"=="" exit /b 0
call :try_env_home "%~1\platform\env_released\env-new"
exit /b 0

:read_user_env
for /f "tokens=2,*" %%A in ('reg query HKCU\Environment /v %~1 2^>nul ^| findstr /i /c:"%~1"') do set "%~1=%%B"
exit /b 0

:find_common_env
for %%D in (C D E F G H) do call :try_common_drive %%D
exit /b 0

:try_common_drive
if not "%RTT_ENV_DIR%"=="" exit /b 0
call :try_env_home "%~1:\Work\RT-ThreadStudio\platform\env_released\env-new"
if not "%RTT_ENV_DIR%"=="" exit /b 0
call :try_env_home "%~1:\RT-ThreadStudio\platform\env_released\env-new"
exit /b 0

:env_not_found
echo [rtt_env] ERROR: RT-Thread Env initialization script was not found.
echo [rtt_env] One-time setup example for this machine:
echo [rtt_env]   powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup_rtt_env.ps1" -StudioHome "D:\Work\RT-ThreadStudio"
echo [rtt_env] Or run setup_rtt_env.ps1 without arguments to auto-detect conventional Studio paths.
exit /b 1

:packages_index_not_found
echo [rtt_env] ERROR: RT-Thread package Kconfig index was not found.
echo [rtt_env] Expected: "%PKGS_ROOT%\packages\Kconfig"
echo [rtt_env] Run "pkgs --upgrade" once in RT-Thread Env, then retry.
exit /b 1

:env_init_failed
echo [rtt_env] ERROR: Env init failed: "%ENV_INIT%"
exit /b %ENV_INIT_RC%

:scons_not_found
echo [rtt_env] ERROR: Env is loaded, but scons is not available in PATH.
exit /b 1

:compiler_not_found
echo [rtt_env] ERROR: arm-none-eabi-gcc.exe was not found after RT-Thread Env initialization.
echo [rtt_env] Run setup_rtt_env.ps1 once or pass -GccBin to select a valid GNU Arm GCC bin directory.
exit /b 1
