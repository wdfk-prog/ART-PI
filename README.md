# ART-PI

Personal STM32H750 ART-Pi project with a clean project/BSP/application/dependency split. RT-Thread, STM32H7 HAL/CMSIS, and third-party dependencies are pinned as Git submodules.

```text
ART-PI/
├── project/
├── BSP/
├── UserSrc/
├── Libraries/
├── tools/
└── docs/
```

Use `project/` as the RT-Thread Studio, SCons/Kconfig, and Keil MDK5 project root.

Before building, add the pinned submodules listed in [docs/SUBMODULES.md](docs/SUBMODULES.md). The machine-readable lock list is [tools/submodules.lock](tools/submodules.lock).

Typical local flow:

```sh
cd project
menuconfig
scons
scons --target=mdk5
```

The migrated project files are prepared for the new layout, but compile/link/target-board validation is intentionally left to the local toolchain after the submodules have been populated.
