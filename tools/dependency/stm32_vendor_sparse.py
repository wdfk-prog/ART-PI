#!/usr/bin/env python3
"""Manage sparse-checkout for ART-PI STM32H7 vendor submodules."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


SCRIPT_DIR = Path(__file__).resolve().parent


@dataclass(frozen=True)
class Profile:
    name: str
    relative_dir: Path
    config: Path


PROFILES = {
    "hal": Profile(
        name="STM32H7 HAL",
        relative_dir=Path("Libraries") / "Vendor" / "STM32" / "stm32h7xx_hal_driver",
        config=SCRIPT_DIR / "stm32h7_hal_sparse_patterns.txt",
    ),
    "cmsis": Profile(
        name="STM32H750 CMSIS device",
        relative_dir=Path("Libraries") / "Vendor" / "STM32" / "cmsis_device_h7",
        config=SCRIPT_DIR / "stm32h750_cmsis_sparse_patterns.txt",
    ),
}


def run_git(repo_dir: Path, *args: str, input_text: Optional[str] = None,
            capture: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["git", "-C", str(repo_dir), *args],
        input=input_text,
        text=True,
        check=True,
        capture_output=capture,
    )


def find_project_root() -> Path:
    for candidate in [SCRIPT_DIR, *SCRIPT_DIR.parents]:
        if all((candidate / profile.relative_dir).is_dir() for profile in PROFILES.values()):
            return candidate
    raise RuntimeError("Cannot find ART-PI project root with STM32 vendor submodules.")


def load_patterns(config: Path) -> List[str]:
    if not config.is_file():
        raise FileNotFoundError(f"Sparse pattern config not found: {config}")

    patterns: List[str] = []
    for line_number, line in enumerate(config.read_text(encoding="utf-8").splitlines(), 1):
        value = line.strip()
        if not value or value.startswith("#"):
            continue
        if "\x00" in value:
            raise ValueError(f"line {line_number}: NUL is not allowed")
        if ".." in Path(value.lstrip("!/")).parts:
            raise ValueError(f"line {line_number}: '..' is not allowed: {value}")
        patterns.append(value)

    if not patterns:
        raise ValueError(f"No sparse patterns configured in: {config}")
    return patterns


def ensure_git_repository(repo_dir: Path, label: str) -> None:
    if not repo_dir.is_dir():
        raise RuntimeError(f"{label} directory does not exist: {repo_dir}")

    inside = run_git(repo_dir, "rev-parse", "--is-inside-work-tree", capture=True)
    if inside.stdout.strip() != "true":
        raise RuntimeError(f"Not a Git work tree: {repo_dir}")

    top = Path(run_git(repo_dir, "rev-parse", "--show-toplevel", capture=True).stdout.strip()).resolve()
    if os.path.normcase(str(top)) != os.path.normcase(str(repo_dir.resolve())):
        raise RuntimeError(f"{label} is not initialized as an independent submodule: {repo_dir}")


def get_status(repo_dir: Path) -> str:
    return run_git(repo_dir, "status", "--porcelain", capture=True).stdout.strip()


def ensure_clean(repo_dir: Path, label: str) -> None:
    status = get_status(repo_dir)
    if status:
        raise RuntimeError(
            f"{label} work tree is not clean. Commit/stash/revert local changes first:\n{status}"
        )


def read_git_config_bool(repo_dir: Path, key: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(repo_dir), "config", "--bool", key],
        text=True,
        capture_output=True,
        check=False,
    )
    return result.stdout.strip() if result.returncode == 0 else ""


def show_profile(profile: Profile, repo_dir: Path) -> None:
    head = run_git(repo_dir, "rev-parse", "HEAD", capture=True).stdout.strip()
    cone = read_git_config_bool(repo_dir, "core.sparseCheckoutCone")
    enabled = read_git_config_bool(repo_dir, "core.sparseCheckout")
    status = get_status(repo_dir)

    print(f"[{profile.name}]")
    print(f"Path:      {repo_dir}")
    print(f"HEAD:      {head}")
    print(f"Work tree: {'clean' if not status else 'DIRTY'}")
    print(f"Sparse:    {'enabled' if enabled == 'true' else 'disabled'} (cone={cone or 'false'})")
    if status:
        print(status)


def apply_profile(profile: Profile, repo_dir: Path) -> None:
    ensure_clean(repo_dir, profile.name)
    patterns = load_patterns(profile.config)
    print(f"[{profile.name}] Config: {profile.config}")

    run_git(repo_dir, "sparse-checkout", "init", "--no-cone")
    run_git(
        repo_dir,
        "sparse-checkout",
        "set",
        "--no-cone",
        "--stdin",
        input_text="\n".join(patterns) + "\n",
    )

    if get_status(repo_dir):
        raise RuntimeError(f"{profile.name} became dirty after sparse-checkout.")
    show_profile(profile, repo_dir)


def disable_profile(profile: Profile, repo_dir: Path) -> None:
    ensure_clean(repo_dir, profile.name)
    run_git(repo_dir, "sparse-checkout", "disable")
    show_profile(profile, repo_dir)


def selected_profiles(target: str) -> List[Profile]:
    if target == "all":
        return [PROFILES["hal"], PROFILES["cmsis"]]
    return [PROFILES[target]]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Manage sparse-checkout for ART-PI STM32H7 HAL/CMSIS submodules."
    )
    parser.add_argument(
        "action",
        nargs="?",
        choices=("apply", "status", "show", "disable"),
        default="status",
    )
    parser.add_argument(
        "--target",
        choices=("all", "hal", "cmsis"),
        default="all",
        help="vendor submodule to operate on (default: all)",
    )
    parser.add_argument(
        "--project-root",
        type=Path,
        default=None,
        help="override ART-PI project root",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        root = args.project_root.expanduser().resolve() if args.project_root else find_project_root()
        for profile in selected_profiles(args.target):
            repo_dir = (root / profile.relative_dir).resolve()
            ensure_git_repository(repo_dir, profile.name)

            if args.action == "show":
                print(f"[{profile.name}] Config: {profile.config}")
                for pattern in load_patterns(profile.config):
                    print(f"  {pattern}")
            elif args.action == "status":
                show_profile(profile, repo_dir)
            elif args.action == "apply":
                apply_profile(profile, repo_dir)
            elif args.action == "disable":
                disable_profile(profile, repo_dir)
            else:
                raise AssertionError(f"Unsupported action: {args.action}")
        return 0
    except (FileNotFoundError, RuntimeError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
