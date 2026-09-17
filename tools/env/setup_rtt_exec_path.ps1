[CmdletBinding()]
param(
    [string]$GccBin
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($GccBin)) {
    $gccCommand = Get-Command arm-none-eabi-gcc.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $gccCommand) {
        throw 'arm-none-eabi-gcc.exe was not found in PATH. Pass -GccBin with the GCC bin directory.'
    }
    $GccBin = Split-Path -Parent $gccCommand.Source
}
elseif (Test-Path -LiteralPath $GccBin -PathType Leaf) {
    $GccBin = Split-Path -Parent (Resolve-Path -LiteralPath $GccBin).Path
}
else {
    $GccBin = (Resolve-Path -LiteralPath $GccBin).Path
}

$gccExe = Join-Path $GccBin 'arm-none-eabi-gcc.exe'
if (-not (Test-Path -LiteralPath $gccExe -PathType Leaf)) {
    throw "arm-none-eabi-gcc.exe was not found under: $GccBin"
}

[Environment]::SetEnvironmentVariable('RTT_EXEC_PATH', $GccBin, 'User')
$env:RTT_EXEC_PATH = $GccBin

Write-Host "[setup_rtt_exec_path] RTT_EXEC_PATH=$GccBin"
Write-Host "[setup_rtt_exec_path] GCC=$gccExe"
Write-Host '[setup_rtt_exec_path] Saved to the Windows User environment.'
Write-Host '[setup_rtt_exec_path] Restart terminals opened before this command.'
