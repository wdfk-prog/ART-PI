@echo off
REM Convenience wrapper: execute RT-Thread Env commands from project/.
pushd "%~dp0project" >nul
call "%~dp0tools\env\rtt_env.cmd" %*
set "RTT_ENV_RC=%errorlevel%"
popd >nul
exit /b %RTT_ENV_RC%
