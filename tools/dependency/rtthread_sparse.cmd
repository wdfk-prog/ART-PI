@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

where python >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    python "%SCRIPT_DIR%rtthread_sparse.py" %*
) else (
    py -3 "%SCRIPT_DIR%rtthread_sparse.py" %*
)

exit /b %ERRORLEVEL%
