# ART-PI

自用 STM32H750 ART-Pi 工程。工程已整理为“项目入口 / BSP / 业务代码 / 依赖”分层，RT-Thread、STM32H7 HAL/CMSIS 和第三方库由 Git Submodule 固定版本管理。

## 目录

```text
ART-PI/
├── project/                 # RT-Thread Studio、Kconfig/SCons、Keil 工程入口
├── BSP/                     # 板级、CubeMX、板级驱动和存储适配
├── UserSrc/                 # applications、用户驱动、业务服务
├── Libraries/               # RT-Thread / Vendor / ThirdParty
├── tools/                   # 依赖锁定清单
└── docs/                    # 工程说明
```

`project/` 是唯一工程入口。`BSP/`、`UserSrc/` 和 `Libraries/` 不再承担 IDE 元数据职责。

当前构建工具链只保留两套：RT-Thread Studio/SCons 使用 GCC，Keil MDK5 使用 ArmClang（ARM Compiler 6）。旧 ARMCC5/IAR 配置已裁剪。

## 子模块

本交付包没有伪造 gitlink。先按 [docs/SUBMODULES.md](docs/SUBMODULES.md) 在你的 Git 仓库中添加并 checkout 到固定 commit。

机器可读版本清单：[`tools/submodules.lock`](tools/submodules.lock)。

迁移与裁剪记录：[`docs/MIGRATION_REPORT.md`](docs/MIGRATION_REPORT.md)。

## RT-Thread Studio

在 RT-Thread Studio 中导入 `project/`。`.project` 使用 linked resources 映射根目录的 `BSP/`、`Libraries/` 和 `UserSrc/`。

当前工程文件已按新目录重写路径；首次同步完全部子模块后，建议做一次 Clean + Build，并检查 Studio 实际使用的 GCC/Env 配置。

## SCons / menuconfig

从工程入口执行：

```sh
cd project
menuconfig
scons
```

`SConstruct` 默认使用 `../Libraries/rt-thread`，也允许通过 `RTT_ROOT` 覆盖。必需子模块没有同步时会直接报错，并提示查看 `docs/SUBMODULES.md`。

## Keil MDK5

Keil 工程位于：

```text
project/project.uvprojx
```

同步子模块后可从 `project/` 重新生成 MDK5 工程：

```sh
cd project
scons --target=mdk5
```

随后打开 `project/project.uvprojx` 并执行 Rebuild All。当前交付未在本环境执行编译、链接或目标板验证，因此第一次本地构建应视为迁移验收的一部分。
