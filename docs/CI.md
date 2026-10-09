# Firmware CI and documentation publishing

## What runs

The `CI - ART-PI` workflow runs on pull requests, pushes to `master`, and manual dispatch. It has two independent jobs:

- **Source checks:** verify project entrypoints, parse the checked-in Python modules, check POSIX shell syntax, and collect a `cppcheck` report for the project-owned `BSP` and `UserSrc` C sources. `cppcheck` findings are informational while the existing baseline is established; its complete report is uploaded as an artifact. CubeMX-generated sources and dependency trees are not analyzed.
- **Firmware:** check out the repository and its recorded Git submodule revisions, install GNU Arm Embedded and SCons 4.0.1, then build from `project/` using `scons`. It verifies the resulting ELF/HEX/BIN files and uploads build logs and firmware artifacts. A failed firmware build fails CI.

The `Docs - Doxygen and Pages` workflow builds the MkDocs portal with `--strict`, generates Doxygen for `BSP` and `UserSrc`, and saves Doxygen warnings as a job artifact. Pull requests only validate the site. A push to `master` or a manual workflow dispatch on `master` deploys the combined site, with Doxygen available under `/api/`.

## First-time GitHub Pages setup

After committing the new files, open the GitHub repository and select:

`Settings` -> `Pages` -> `Build and deployment` -> `Source` -> `GitHub Actions`

The first Pages deployment cannot succeed until Pages is enabled and configured this way. No personal access token is required.

After a successful default-branch deployment, the expected URL is:

- Portal: <https://wdfk-prog.github.io/ART-PI/>
- Doxygen: <https://wdfk-prog.github.io/ART-PI/api/>

## Local equivalents

Use the actual checked-in Git submodules; do not substitute arbitrary upstream dependency versions. The Linux build resolves aggregation scripts from the lower-case `libraries/` directory and dependency sources from the upper-case `Libraries/` directory.

```sh
git submodule update --init
python3 -m pip install 'scons==4.0.1'
cd project
RTT_CC=gcc scons -j2
```

A GNU Arm Embedded toolchain and the required RT-Thread/STM32 dependencies must be installed for the firmware build. For the documentation portal, use:

```sh
python3 -m pip install -r requirements-docs.txt
mkdocs build --strict
mkdir -p build/doxygen
doxygen Doxyfile
mkdir -p site/api
cp -a build/doxygen/html/. site/api/
touch site/.nojekyll
```

Install Doxygen and Graphviz before running the documentation commands. Do not mistake local HTML generation for a completed GitHub Pages deployment.

## Validation boundaries

- Python/Shell syntax and `cppcheck` are not a substitute for cross-compilation or target-board testing.
- The firmware CI build validates ARM GCC linking and artifact generation, not peripheral timing, Bootloader handoff, or HIL behavior.
- Doxygen warnings are reported without enforcing documentation coverage on legacy code.
- The documentation workflow never modifies source, creates releases, or publishes firmware.
