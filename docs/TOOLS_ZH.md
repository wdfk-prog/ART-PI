# tools 工具使用说明

本文说明 `tools/` 下各辅助工具的用途、前置条件和常用命令。除特别说明外，示例均从 ART-PI 仓库根目录开始执行。

## 目录概览

```text
tools/
├── env/                         # Windows RT-Thread Env / GNU Arm GCC 环境初始化
├── dependency/                  # RT-Thread、STM32 HAL/CMSIS 稀疏检出
├── firmware/                    # 固件版本提取、命名、HEX 合并
└── submodules.lock              # 子模块路径、URL、分支和固定 commit 清单
```

`tools/submodules.lock` 不是可执行脚本，格式为：

```text
path|url|branch|commit
```

它用于记录工程依赖的固定版本。子模块初始化与 checkout 应以仓库实际的 Git Submodule 配置和该锁定清单为准。

## 1. `tools/env`：RT-Thread 构建环境

这组脚本用于 Windows 下定位 RT-Thread Env、GNU Arm GCC，并为 SCons/menuconfig 准备环境。

### 1.1 首次机器配置：`setup_rtt_env.ps1`

通常每台 Windows 机器只需要执行一次。脚本会设置当前用户级环境变量：

- `RTT_ENV_HOME`：RT-Thread Env 目录。
- `RTT_EXEC_PATH`：包含 `arm-none-eabi-gcc.exe` 的 GNU Arm GCC `bin` 目录。

如果 RT-Thread Studio 安装在常见路径（例如 `<drive>:\Work\RT-ThreadStudio` 或 `<drive>:\RT-ThreadStudio`），可以直接尝试自动探测：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\env\setup_rtt_env.ps1
```

也可以明确指定 RT-Thread Studio 根目录：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\env\setup_rtt_env.ps1 `
  -StudioHome 'D:\Work\RT-ThreadStudio'
```

或者直接指定 Env 目录和 GCC `bin` 目录：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\env\setup_rtt_env.ps1 `
  -EnvHome 'D:\Work\RT-ThreadStudio\platform\env_released\env-new' `
  -GccBin 'D:\path\to\gcc\bin'
```

脚本会优先使用显式参数，然后检查已有的用户环境变量、RT-Thread Env 自带 GCC，以及当前 `PATH` 中的 `arm-none-eabi-gcc.exe`。

如果提示缺少 RT-Thread package Kconfig 索引，请在 RT-Thread Env 中先执行：

```text
pkgs --upgrade
```

### 1.2 只调整 GCC：`setup_rtt_exec_path.ps1`

当 RT-Thread Env 已可用，只需要修正 GCC 路径时使用：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\env\setup_rtt_exec_path.ps1 `
  -GccBin 'D:\path\to\gcc\bin'
```

如果 `arm-none-eabi-gcc.exe` 已在 `PATH` 中，可省略 `-GccBin`：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\env\setup_rtt_exec_path.ps1
```

该脚本把 `RTT_EXEC_PATH` 保存为 Windows 当前用户环境变量。对命令执行前已经打开的终端，建议重新打开终端，或重新加载环境。

### 1.3 每次进入工程：`rtt_env.cmd`

`rtt_env.cmd` 用于当前 CMD 会话。它会：

1. 查找并加载 `RTT_ENV_HOME`/RT-Thread Studio 中的 Env 初始化脚本。
2. 检查 `PKGS_ROOT` 和 package Kconfig 索引。
3. 检查 `scons` 是否可用。
4. 恢复或查找有效的 `RTT_EXEC_PATH`。
5. 校验 `arm-none-eabi-gcc.exe` 是否存在。

ART-PI 的 SCons 工程入口在 `project/`，因此推荐先进入 `project/` 再加载环境：

```bat
cd project
call ..\tools\env\rtt_env.cmd
menuconfig
scons
```

也可以把命令直接交给脚本执行：

```bat
cd project
call ..\tools\env\rtt_env.cmd scons
```

交互式 CMD 中可直接执行脚本；如果从另一个 `.bat`/`.cmd` 批处理脚本中调用，为了在执行完成后返回调用者，应使用 `call`。如果当前 CMD 已加载另一套不同路径的 RT-Thread Env，脚本会拒绝混用并返回错误。

## 2. `tools/dependency`：子模块稀疏检出

稀疏检出用于减少 RT-Thread、STM32 HAL 和 CMSIS 子模块工作树中不需要的文件。执行 `apply` 或 `disable` 前，脚本要求目标子模块工作树保持 clean；有本地修改时会直接拒绝操作。

Windows 推荐使用 `.cmd` 包装脚本。包装脚本会优先调用 `python`，找不到时尝试 `py -3`。

### 2.1 RT-Thread：`rtthread_sparse.cmd`

默认子模块路径：

```text
Libraries/rt-thread
```

默认配置文件：

```text
tools/dependency/rtthread_sparse_paths.txt
```

常用命令：

```bat
rem 查看当前 sparse-checkout 与工作树状态；这也是无参数时的默认动作
tools\dependency\rtthread_sparse.cmd status

rem 只显示将保留的路径配置，不修改 sparse-checkout
tools\dependency\rtthread_sparse.cmd show

rem 应用 sparse-checkout
tools\dependency\rtthread_sparse.cmd apply

rem 关闭 sparse-checkout，恢复完整 RT-Thread 工作树
tools\dependency\rtthread_sparse.cmd disable
```

如需覆盖默认路径：

```bat
tools\dependency\rtthread_sparse.cmd status --rt-thread-dir D:\work\rt-thread
```

如需使用另一份保留路径清单：

```bat
tools\dependency\rtthread_sparse.cmd apply --config D:\work\my_sparse_paths.txt
```

该脚本使用 Git cone mode；配置文件中的每一行是相对于 `Libraries/rt-thread` 的目录路径。

### 2.2 STM32 HAL/CMSIS：`stm32_vendor_sparse.cmd`

该工具管理两个 Vendor 子模块：

- `hal`：`Libraries/Vendor/STM32/stm32h7xx_hal_driver`
- `cmsis`：`Libraries/Vendor/STM32/cmsis_device_h7`

默认 `--target all`，即同时处理两者。常用命令：

```bat
rem 查看 HAL 与 CMSIS 的状态
tools\dependency\stm32_vendor_sparse.cmd status

rem 查看当前配置会保留哪些文件
tools\dependency\stm32_vendor_sparse.cmd show --target all

rem 应用 HAL + CMSIS sparse-checkout
tools\dependency\stm32_vendor_sparse.cmd apply

rem 只处理 HAL
tools\dependency\stm32_vendor_sparse.cmd apply --target hal

rem 只处理 CMSIS
tools\dependency\stm32_vendor_sparse.cmd apply --target cmsis

rem 关闭两者 sparse-checkout
tools\dependency\stm32_vendor_sparse.cmd disable
```

如果脚本无法从自身位置识别工程根目录，可显式指定：

```bat
tools\dependency\stm32_vendor_sparse.cmd status --project-root D:\work\ART-PI
```

HAL/CMSIS 使用 non-cone sparse-checkout，实际保留规则分别来自：

```text
tools/dependency/stm32h7_hal_sparse_patterns.txt
tools/dependency/stm32h750_cmsis_sparse_patterns.txt
```

### 2.3 直接运行 Python 版本

`.cmd` 只是 Windows 包装层，也可以直接运行 Python 脚本：

```sh
python tools/dependency/rtthread_sparse.py status
python tools/dependency/stm32_vendor_sparse.py status --target all
```

两者都依赖本机 `git`，并要求目标目录已经初始化为独立 Git 子模块工作树。

## 3. `tools/firmware`：固件产物处理

这些脚本是 POSIX `sh` 脚本，可在 Linux、WSL、Git Bash/MSYS2 等具备兼容 `sh` 的环境中使用。

> 注意：当前上传包中的 `tools/firmware/*.sh` 保留了 CRLF 行尾。在本次 Linux `/bin/sh` 校验中，原文件会因 `\r` 导致语法解析失败。若在 Linux/WSL/严格 POSIX `sh` 下使用，请先确保这些脚本以 LF 行尾 checkout（例如由仓库 `.gitattributes` 统一控制）或转换为 LF；本文档任务未改动脚本本身。

### 3.1 组合固件名称：`name_joiner.sh`

按固定顺序组合非空字段：

```text
<prefix>_<middle>_<version>_<suffix>
```

示例：

```sh
sh tools/firmware/name_joiner.sh \
  --prefix ART-PI \
  --middle app \
  --version 1.2.3 \
  --suffix release
```

输出：

```text
ART-PI_app_1.2.3_release
```

任意字段都可以省略，但至少要提供一个字段。

### 3.2 从头文件提取版本：`get_version_from_header.sh`

从指定头文件中读取一个简单的 `#define` 值。默认宏名为 `VERSION`：

```sh
sh tools/firmware/get_version_from_header.sh \
  --header-file <path/to/version.h>
```

指定其他宏名：

```sh
sh tools/firmware/get_version_from_header.sh \
  --header-file <path/to/version.h> \
  --macro FW_VERSION
```

例如头文件中存在：

```c
#define FW_VERSION "1.2.3"
```

脚本输出：

```text
1.2.3
```

该工具只提取宏后面的文本，不执行 C 预处理或宏展开。

### 3.3 合并 Bootloader 与 APP HEX：`merge_hex_fast.sh`

基本命令：

```sh
sh tools/firmware/merge_hex_fast.sh \
  --boot <path/to/bootloader.hex> \
  --app <path/to/app.hex> \
  --output <path/to/merged.hex>
```

脚本会校验 Intel HEX 的基本语法、记录长度、checksum、EOF 记录和数据记录，并计算两个输入文件的数据地址范围。

默认行为：

- 两个 HEX 数据地址范围发生重叠时返回错误。
- `--start-addr first`：保留第一个输入（Bootloader）的首个 start-address 记录。
- 合并第二个 HEX 前会重置扩展地址基址，避免继承第一个文件的地址状态。
- 输出先写临时文件，再移动到目标路径。

可选 start-address 策略：

```sh
--start-addr first
--start-addr second
--start-addr none
```

仅在确认地址重叠是设计行为时才使用：

```sh
--allow-overlap
```

安静模式：

```sh
--quiet
```

### 3.4 固件转换与统一命名：`firmware_namer.sh`

这是 `tools/firmware` 的主入口。它可以：

- 用 `arm-none-eabi-objcopy` 把 ELF 转成 HEX 和 BIN。
- 给 HEX/BIN 生成统一产品名。
- 在提供 Bootloader HEX 时，把 Bootloader 与 APP HEX 合并成最终命名的 HEX。

至少需要提供 `--prefix`、`--middle`、`--version`、`--suffix` 中的一个命名字段；四者都为空时，内部的 `name_joiner.sh` 会返回错误。

最小示例，直接处理 ELF：

```sh
sh tools/firmware/firmware_namer.sh \
  --project-root . \
  --prefix ART-PI \
  --middle app \
  --version 1.2.3 \
  --elf <path/to/firmware.elf>
```

如果 `<path/to/firmware.elf>` 存在，脚本默认执行：

```text
arm-none-eabi-objcopy -O ihex   <elf> <same-name>.hex
arm-none-eabi-objcopy -O binary <elf> <same-name>.bin
```

然后在原 HEX/BIN 所在目录复制或生成：

```text
ART-PI_app_1.2.3.hex
ART-PI_app_1.2.3.bin
```

当 `--elf` 指向有效文件时，脚本会把 HEX/BIN 输入路径固定为该 ELF 的同名 `.hex`/`.bin`，因此同时传入的 `--hex` 或 `--bin` 值不会作为最终输入路径使用。

如果不需要从 ELF 运行 `objcopy`，可直接提供已有产物：

```sh
sh tools/firmware/firmware_namer.sh \
  --prefix ART-PI \
  --version 1.2.3 \
  --hex <path/to/app.hex> \
  --bin <path/to/app.bin>
```

需要使用自定义 `objcopy` 命令或完整路径：

```sh
--objcopy <path/to/arm-none-eabi-objcopy>
```

`--no-objcopy` 会跳过 ELF 转换；此时如果同时提供 `--elf`，至少一个同名 `.hex` 或 `.bin` 产物必须已经存在，否则脚本找不到可处理的固件产物。

合并 Bootloader：

```sh
sh tools/firmware/firmware_namer.sh \
  --prefix ART-PI \
  --middle app \
  --version 1.2.3 \
  --hex <path/to/app.hex> \
  --boot-hex <path/to/bootloader.hex> \
  --merged-start-addr first
```

`--merged-start-addr` 可取 `first`、`second` 或 `none`。地址范围重叠默认报错；只有确认设计允许重叠时才使用 `--allow-merge-overlap`。

注意：`--merge-hex <path>` 参数用于覆盖“HEX 合并脚本”的路径，默认值是 `tools/firmware/merge_hex_fast.sh`，它不是最终合并 HEX 的输出路径。

### 3.5 从头文件版本号到最终固件名

可以把两个工具串起来：

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

## 4. 推荐使用顺序

首次准备 Windows 开发机：

```text
setup_rtt_env.ps1
    -> 配置 RTT_ENV_HOME / RTT_EXEC_PATH
```

同步并初始化 Git Submodule 后，根据需要裁剪依赖工作树：

```text
rtthread_sparse.cmd apply
stm32_vendor_sparse.cmd apply
```

日常构建：

```text
cd project
call ..\tools\env\rtt_env.cmd
menuconfig / scons
```

构建得到 ELF/HEX/BIN 后，再按发布命名或 Bootloader 合并需求使用 `tools/firmware/`。

## 5. 常见错误

| 现象 | 原因/处理 |
| --- | --- |
| `RT-Thread Env initialization script was not found` | 先运行 `setup_rtt_env.ps1`，必要时显式传入 `-StudioHome` 或 `-EnvHome`。 |
| `package Kconfig index was not found` | 在 RT-Thread Env 中运行一次 `pkgs --upgrade`。 |
| `scons is not available in PATH` | Env 未正确加载，检查 `RTT_ENV_HOME` 对应的 Env 初始化脚本。 |
| `arm-none-eabi-gcc.exe was not found` | 用 `setup_rtt_env.ps1 -GccBin ...` 或 `setup_rtt_exec_path.ps1 -GccBin ...` 指定 GCC。 |
| sparse 工具提示 work tree `DIRTY` | `apply`/`disable` 前先 commit、stash 或还原目标子模块的本地修改。 |
| sparse 工具提示不是独立 Git work tree | 先正确初始化对应 Git Submodule。 |
| `merge_hex_fast.sh` 报地址范围重叠 | 检查 Bootloader/APP 链接地址；不要用 `--allow-overlap` 掩盖非预期链接布局。 |
| `firmware_namer.sh` 找不到固件产物 | 确认 `--elf`、`--hex`、`--bin` 路径；使用 `--no-objcopy` 且同时提供 `--elf` 时，至少一个同名 HEX/BIN 产物需已存在。 |
