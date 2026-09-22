# tools Usage

This page documents the helper utilities under `tools/`. Unless noted otherwise, examples start from the ART-PI repository root.

## Layout

```text
tools/
├── env/                         # Windows RT-Thread Env / GNU Arm GCC setup
├── dependency/                  # Sparse-checkout helpers for RT-Thread and STM32 vendor code
├── firmware/                    # Firmware version, naming, and Intel HEX helpers
└── submodules.lock              # Pinned submodule path/URL/branch/commit list
```

`tools/submodules.lock` is data rather than an executable script. Each non-comment line uses:

```text
path|url|branch|commit
```

## 1. RT-Thread build environment

### One-time machine setup: `setup_rtt_env.ps1`

Run this once on a Windows development machine to persist the user-level `RTT_ENV_HOME` and `RTT_EXEC_PATH` variables.

Automatic discovery of conventional RT-Thread Studio locations:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\env\setup_rtt_env.ps1
```

Specify the Studio root explicitly when needed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\env\setup_rtt_env.ps1 `
  -StudioHome 'D:\Work\RT-ThreadStudio'
```

Or specify the Env and GCC directories directly:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\env\setup_rtt_env.ps1 `
  -EnvHome 'D:\Work\RT-ThreadStudio\platform\env_released\env-new' `
  -GccBin 'D:\path\to\gcc\bin'
```

If the package Kconfig index is missing, open RT-Thread Env and run:

```text
pkgs --upgrade
```

### Change only the GCC path: `setup_rtt_exec_path.ps1`

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\env\setup_rtt_exec_path.ps1 `
  -GccBin 'D:\path\to\gcc\bin'
```

When `arm-none-eabi-gcc.exe` is already in `PATH`, `-GccBin` may be omitted.

### Daily CMD session: `rtt_env.cmd`

The SCons project root is `project/`, so load the environment after changing to that directory:

```bat
cd project
call ..\tools\env\rtt_env.cmd
menuconfig
scons
```

A command can also be passed through the loader:

```bat
cd project
call ..\tools\env\rtt_env.cmd scons
```

At an interactive CMD prompt the script can be run directly. When invoking it from another `.bat`/`.cmd` file, use `call` so execution resumes in the caller. The script rejects an already-loaded Env when it points to a different installation.

## 2. Dependency sparse-checkout helpers

The sparse-checkout helpers require initialized Git submodules. `apply` and `disable` require the target submodule work tree to be clean.

### RT-Thread

Default submodule: `Libraries/rt-thread`.

```bat
tools\dependency\rtthread_sparse.cmd status
tools\dependency\rtthread_sparse.cmd show
tools\dependency\rtthread_sparse.cmd apply
tools\dependency\rtthread_sparse.cmd disable
```

Optional overrides:

```bat
tools\dependency\rtthread_sparse.cmd status --rt-thread-dir D:\work\rt-thread
tools\dependency\rtthread_sparse.cmd apply --config D:\work\my_sparse_paths.txt
```

The retained directory list is stored in `tools/dependency/rtthread_sparse_paths.txt` and is applied with Git cone-mode sparse-checkout.

### STM32 HAL and CMSIS

Targets are:

- `hal`: `Libraries/Vendor/STM32/stm32h7xx_hal_driver`
- `cmsis`: `Libraries/Vendor/STM32/cmsis_device_h7`

The default target is `all`.

```bat
tools\dependency\stm32_vendor_sparse.cmd status
tools\dependency\stm32_vendor_sparse.cmd show --target all
tools\dependency\stm32_vendor_sparse.cmd apply
tools\dependency\stm32_vendor_sparse.cmd apply --target hal
tools\dependency\stm32_vendor_sparse.cmd apply --target cmsis
tools\dependency\stm32_vendor_sparse.cmd disable
```

When root auto-detection is not possible:

```bat
tools\dependency\stm32_vendor_sparse.cmd status --project-root D:\work\ART-PI
```

These profiles use non-cone sparse-checkout patterns from:

```text
tools/dependency/stm32h7_hal_sparse_patterns.txt
tools/dependency/stm32h750_cmsis_sparse_patterns.txt
```

The Python implementations can also be called directly:

```sh
python tools/dependency/rtthread_sparse.py status
python tools/dependency/stm32_vendor_sparse.py status --target all
```

## 3. Firmware helpers

The scripts under `tools/firmware/` are POSIX `sh` scripts and can be used from Linux, WSL, Git Bash/MSYS2, or another compatible shell.

> Note: the uploaded package currently preserves CRLF line endings for `tools/firmware/*.sh`. In this validation environment, the original files fail Linux `/bin/sh` parsing because of the retained `\r`. Ensure these scripts are checked out with LF line endings (for example via repository `.gitattributes`) or convert them before running under Linux/WSL/strict POSIX `sh`. This documentation-only change does not modify the scripts.

### `name_joiner.sh`

Joins non-empty fields in this order:

```text
<prefix>_<middle>_<version>_<suffix>
```

Example:

```sh
sh tools/firmware/name_joiner.sh \
  --prefix ART-PI \
  --middle app \
  --version 1.2.3 \
  --suffix release
```

Result: `ART-PI_app_1.2.3_release`.

### `get_version_from_header.sh`

Extract a simple `#define` value from a header. The default macro name is `VERSION`.

```sh
sh tools/firmware/get_version_from_header.sh \
  --header-file <path/to/version.h> \
  --macro FW_VERSION
```

For `#define FW_VERSION "1.2.3"`, the output is `1.2.3`. The script does not run the C preprocessor or expand macros.

### `merge_hex_fast.sh`

Merge a Bootloader and APP Intel HEX file:

```sh
sh tools/firmware/merge_hex_fast.sh \
  --boot <path/to/bootloader.hex> \
  --app <path/to/app.hex> \
  --output <path/to/merged.hex>
```

The script validates Intel HEX syntax, record length, checksum, EOF/data records, and data ranges. Overlapping data ranges fail by default.

Start-address policy:

```text
--start-addr first   # default, keep the first input start-address record
--start-addr second  # keep the second input start-address record
--start-addr none    # omit start-address records
```

Use `--allow-overlap` only when overlapping address ranges are intentional. `--quiet` suppresses informational output.

### `firmware_namer.sh`

This is the main firmware artifact helper. It can convert an ELF to HEX/BIN with `arm-none-eabi-objcopy`, rename artifacts consistently, and optionally merge a Bootloader HEX with the APP HEX.

At least one naming field among `--prefix`, `--middle`, `--version`, and `--suffix` is required; if all four are empty, the internal `name_joiner.sh` call fails.

```sh
sh tools/firmware/firmware_namer.sh \
  --project-root . \
  --prefix ART-PI \
  --middle app \
  --version 1.2.3 \
  --elf <path/to/firmware.elf>
```

With an ELF input and objcopy enabled, the script creates same-stem `.hex` and `.bin` files, then writes the final named artifacts in the same directories.

When `--elf` points to an existing file, the script sets the HEX/BIN input paths to the ELF same-stem `.hex`/`.bin`; any simultaneously supplied `--hex` or `--bin` values are therefore not used as the final input paths.

Existing artifacts can be used directly:

```sh
sh tools/firmware/firmware_namer.sh \
  --prefix ART-PI \
  --version 1.2.3 \
  --hex <path/to/app.hex> \
  --bin <path/to/app.bin>
```

Use `--objcopy <command-or-path>` to override objcopy. `--no-objcopy` skips ELF conversion; when an ELF is still supplied, at least one corresponding same-stem `.hex` or `.bin` artifact must already exist.

Bootloader merge example:

```sh
sh tools/firmware/firmware_namer.sh \
  --prefix ART-PI \
  --middle app \
  --version 1.2.3 \
  --hex <path/to/app.hex> \
  --boot-hex <path/to/bootloader.hex> \
  --merged-start-addr first
```

`--merged-start-addr` accepts `first`, `second`, or `none`. Use `--allow-merge-overlap` only when overlap is intentional.

Important: `--merge-hex <path>` overrides the HEX merge **script path**. It does not specify the merged HEX output file. The default merge script is `tools/firmware/merge_hex_fast.sh`.

### Build a name from a version header

```sh
VERSION="$(sh tools/firmware/get_version_from_header.sh \
  --header-file <path/to/version.h> \
  --macro FW_VERSION)"

sh tools/firmware/firmware_namer.sh \
  --project-root . \
  --prefix ART-PI \
  --middle app \
  --version "$VERSION" \
  --elf <path/to/firmware.elf>
```

## 4. Recommended order

On a new Windows development machine, run `setup_rtt_env.ps1` once. After Git submodules are initialized, optionally apply the RT-Thread and STM32 vendor sparse-checkout profiles. For daily builds, enter `project/`, call `rtt_env.cmd`, then run `menuconfig`/`scons`. Use the firmware helpers after build artifacts have been produced.

## 5. Common failures

| Symptom | Action |
| --- | --- |
| RT-Thread Env initialization script not found | Run `setup_rtt_env.ps1`; pass `-StudioHome` or `-EnvHome` if auto-discovery cannot find the installation. |
| Package Kconfig index not found | Run `pkgs --upgrade` once from RT-Thread Env. |
| `scons` not found | Verify that the selected RT-Thread Env initializes correctly. |
| `arm-none-eabi-gcc.exe` not found | Set `-GccBin` with `setup_rtt_env.ps1` or `setup_rtt_exec_path.ps1`. |
| Sparse helper reports `DIRTY` | Commit, stash, or revert changes in the target submodule before `apply`/`disable`. |
| Sparse helper says the directory is not an independent work tree | Initialize the corresponding Git submodule first. |
| HEX merge reports overlapping data ranges | Verify Bootloader/APP link addresses; do not hide an unintended layout with `--allow-overlap`. |
| `firmware_namer.sh` cannot find an artifact | Check `--elf`/`--hex`/`--bin`; with `--no-objcopy` plus `--elf`, at least one same-stem HEX/BIN artifact must already exist. |
