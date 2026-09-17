[CmdletBinding()]
param(
    [string]$EnvHome,
    [string]$GccBin,
    [string]$StudioHome
)

$ErrorActionPreference = 'Stop'

function Get-UserEnvironmentVariable {
    param([Parameter(Mandatory = $true)][string]$Name)

    return [Environment]::GetEnvironmentVariable($Name, 'User')
}

function Test-RttEnvHome {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or
        -not (Test-Path -LiteralPath $Path -PathType Container)) {
        return $false
    }

    $resolved = (Resolve-Path -LiteralPath $Path).Path
    return (Test-Path -LiteralPath (Join-Path $resolved 'tools\bin\env-init.bat') -PathType Leaf) -or
           (Test-Path -LiteralPath (Join-Path $resolved 'env.bat') -PathType Leaf)
}

function Add-EnvCandidate {
    param(
        [Parameter(Mandatory = $true)]$Candidates,
        [string]$Path
    )

    if (-not [string]::IsNullOrWhiteSpace($Path) -and -not $Candidates.Contains($Path)) {
        $Candidates.Add($Path)
    }
}

function Get-CommonEnvCandidates {
    $candidates = New-Object System.Collections.Generic.List[string]

    foreach ($drive in [System.IO.DriveInfo]::GetDrives()) {
        if (-not $drive.IsReady) {
            continue
        }

        $root = $drive.RootDirectory.FullName
        Add-EnvCandidate -Candidates $candidates -Path (Join-Path $root 'Work\RT-ThreadStudio\platform\env_released\env-new')
        Add-EnvCandidate -Candidates $candidates -Path (Join-Path $root 'RT-ThreadStudio\platform\env_released\env-new')
    }

    return $candidates
}

function Resolve-EnvHome {
    param(
        [string]$ExplicitEnvHome,
        [string]$ExplicitStudioHome
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitEnvHome)) {
        if (-not (Test-RttEnvHome -Path $ExplicitEnvHome)) {
            $detected = Get-CommonEnvCandidates | Where-Object { Test-RttEnvHome -Path $_ } | Select-Object -First 1
            if ($detected) {
                throw "RT-Thread Env directory does not exist or has no init script: $ExplicitEnvHome`nDetected Env: $detected"
            }
            throw "RT-Thread Env directory does not exist or has no init script: $ExplicitEnvHome"
        }
        return (Resolve-Path -LiteralPath $ExplicitEnvHome).Path
    }

    $candidates = New-Object System.Collections.Generic.List[string]
    Add-EnvCandidate -Candidates $candidates -Path $env:RTT_ENV_HOME
    Add-EnvCandidate -Candidates $candidates -Path (Get-UserEnvironmentVariable -Name 'RTT_ENV_HOME')
    Add-EnvCandidate -Candidates $candidates -Path $env:ENV_ROOT

    $studioCandidates = New-Object System.Collections.Generic.List[string]
    Add-EnvCandidate -Candidates $studioCandidates -Path $ExplicitStudioHome
    Add-EnvCandidate -Candidates $studioCandidates -Path $env:RTT_STUDIO_HOME
    Add-EnvCandidate -Candidates $studioCandidates -Path (Get-UserEnvironmentVariable -Name 'RTT_STUDIO_HOME')

    foreach ($studio in $studioCandidates) {
        Add-EnvCandidate -Candidates $candidates -Path (Join-Path $studio 'platform\env_released\env-new')
    }

    foreach ($candidate in Get-CommonEnvCandidates) {
        Add-EnvCandidate -Candidates $candidates -Path $candidate
    }

    foreach ($candidate in $candidates) {
        if (Test-RttEnvHome -Path $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw @'
RT-Thread Env was not found.
Run this script with the Studio root or Env directory once, for example:
  .\tools\env\setup_rtt_env.ps1 -StudioHome 'D:\Work\RT-ThreadStudio'
or:
  .\tools\env\setup_rtt_env.ps1 -EnvHome 'D:\Work\RT-ThreadStudio\platform\env_released\env-new'
'@
}

function Resolve-GccBin {
    param(
        [string]$ExplicitGccBin,
        [Parameter(Mandatory = $true)][string]$ResolvedEnvHome
    )

    $candidates = New-Object System.Collections.Generic.List[string]

    if (-not [string]::IsNullOrWhiteSpace($ExplicitGccBin)) {
        if (Test-Path -LiteralPath $ExplicitGccBin -PathType Leaf) {
            $resolvedExplicit = Split-Path -Parent (Resolve-Path -LiteralPath $ExplicitGccBin).Path
        }
        elseif (Test-Path -LiteralPath $ExplicitGccBin -PathType Container) {
            $resolvedExplicit = (Resolve-Path -LiteralPath $ExplicitGccBin).Path
        }
        else {
            throw "GNU Arm GCC path does not exist: $ExplicitGccBin"
        }

        if (-not (Test-Path -LiteralPath (Join-Path $resolvedExplicit 'arm-none-eabi-gcc.exe') -PathType Leaf)) {
            throw "arm-none-eabi-gcc.exe was not found under: $resolvedExplicit"
        }
        return $resolvedExplicit
    }

    Add-EnvCandidate -Candidates $candidates -Path $env:RTT_EXEC_PATH
    Add-EnvCandidate -Candidates $candidates -Path (Get-UserEnvironmentVariable -Name 'RTT_EXEC_PATH')

    # RT-Thread Env for Windows ships its GNU Arm toolchain here. This makes the
    # one-time setup work without requiring a separate GCC path argument.
    Add-EnvCandidate -Candidates $candidates -Path (Join-Path $ResolvedEnvHome 'tools\gnu_gcc\arm_gcc\mingw\bin')

    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }

        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            $resolvedCandidate = Split-Path -Parent (Resolve-Path -LiteralPath $candidate).Path
        }
        elseif (Test-Path -LiteralPath $candidate -PathType Container) {
            $resolvedCandidate = (Resolve-Path -LiteralPath $candidate).Path
        }
        else {
            continue
        }

        if (Test-Path -LiteralPath (Join-Path $resolvedCandidate 'arm-none-eabi-gcc.exe') -PathType Leaf) {
            return $resolvedCandidate
        }
    }

    $gccCommand = Get-Command arm-none-eabi-gcc.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($gccCommand) {
        return (Split-Path -Parent $gccCommand.Source)
    }

    throw @'
arm-none-eabi-gcc.exe was not found.
Pass its bin directory once with -GccBin, or verify that the selected RT-Thread Env contains its GNU Arm toolchain.
'@
}

$resolvedEnvHome = Resolve-EnvHome -ExplicitEnvHome $EnvHome -ExplicitStudioHome $StudioHome
$resolvedGccBin = Resolve-GccBin -ExplicitGccBin $GccBin -ResolvedEnvHome $resolvedEnvHome

[Environment]::SetEnvironmentVariable('RTT_ENV_HOME', $resolvedEnvHome, 'User')
[Environment]::SetEnvironmentVariable('RTT_EXEC_PATH', $resolvedGccBin, 'User')

# Keep the current PowerShell session usable as well. Other already-running
# parent processes cannot be updated; rtt_env.cmd reads User values from HKCU
# when its inherited environment is stale.
$env:RTT_ENV_HOME = $resolvedEnvHome
$env:RTT_EXEC_PATH = $resolvedGccBin

$packageIndex = Join-Path $resolvedEnvHome 'packages\packages\Kconfig'
Write-Host '[setup_rtt_env] Windows User environment configured.'
Write-Host "[setup_rtt_env] RTT_ENV_HOME=$resolvedEnvHome"
Write-Host "[setup_rtt_env] RTT_EXEC_PATH=$resolvedGccBin"
if (Test-Path -LiteralPath $packageIndex -PathType Leaf) {
    Write-Host "[setup_rtt_env] Package index=$packageIndex"
}
else {
    Write-Warning "RT-Thread package index was not found at: $packageIndex. Run 'pkgs --upgrade' from RT-Thread Env if pyconfig reports a missing package Kconfig."
}
Write-Host '[setup_rtt_env] This is one-time Windows User configuration shared by all RT-Thread projects.'
