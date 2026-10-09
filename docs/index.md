# ART-PI Documentation

ART-PI is an STM32H750 ART-Pi firmware project based on RT-Thread. The repository separates its SCons project entrypoint, board support, application code, and pinned third-party dependencies.

## Documentation

- [CI, firmware build, and Pages setup](CI.md)
- [Tooling guide (Chinese)](TOOLS_ZH.md)
- [Tooling guide (English)](TOOLS.md)
- [Project overview (Chinese, GitHub)](https://github.com/wdfk-prog/ART-PI/blob/master/README_ZH.md)
- [ART-Pi manuals and hardware references (GitHub)](https://github.com/wdfk-prog/ART-PI/tree/master/documents)

## C/C++ API and source browser

<a href="api/index.html">Open the Doxygen API reference</a> to browse board support, drivers, application services, declarations, source files, and cross references.

The API reference is generated at deployment time under the same GitHub Pages site at `/api/`. Vendor, RT-Thread and CubeMX-generated code are excluded so that the reference focuses on project-owned logic.

## Repository structure

| Path | Purpose |
| --- | --- |
| `project/` | SCons, Kconfig, RT-Thread Studio, and build configuration |
| `BSP/` | Board support, CubeMX integration, device drivers, storage |
| `UserSrc/` | Application services and user drivers |
| `Libraries/` | RT-Thread, STM32 HAL/CMSIS and other pinned submodules |
| `tools/` | Environment, dependency, and firmware helpers |
| `docs/` | This documentation portal |
