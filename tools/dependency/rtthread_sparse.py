#!/usr/bin/env python3
"""Manage sparse-checkout for the ART-PI RT-Thread submodule."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path, PurePosixPath
from typing import List, Optional, Set


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_CONFIG = SCRIPT_DIR / "rtthread_sparse_paths.txt"
DEFAULT_RTTHREAD_RELATIVE = Path("Libraries") / "rt-thread"


def run_git(rtthread_dir: Path, *args: str, input_text: Optional[str] = None,
            capture: bool = False) -> subprocess.CompletedProcess:
    cmd = ["git", "-C", str(rtthread_dir), *args]
    return subprocess.run(
        cmd,
        input=input_text,
        text=True,
        check=True,
        capture_output=capture,
    )


def find_project_root(rtthread_relative: Path) -> Path:
    candidates = [SCRIPT_DIR, *SCRIPT_DIR.parents]
    for candidate in candidates:
        if (candidate / rtthread_relative).is_dir():
            return candidate
    raise RuntimeError(
        f"Cannot find project root containing '{rtthread_relative.as_posix()}'."
    )


def normalize_sparse_path(raw: str, line_number: int) -> str:
    value = raw.strip().replace("\\", "/")
    path = PurePosixPath(value)

    if path.is_absolute() or value.startswith("//") or (len(value) >= 2 and value[1] == ":"):
        raise ValueError(f"line {line_number}: absolute path is not allowed: {raw}")
    if ".." in path.parts:
        raise ValueError(f"line {line_number}: '..' is not allowed: {raw}")
    if value in ("", "."):
        raise ValueError(f"line {line_number}: empty/root path is not allowed")

    return path.as_posix().rstrip("/")


def load_sparse_paths(config_path: Path) -> List[str]:
    if not config_path.is_file():
        raise FileNotFoundError(f"Sparse path config not found: {config_path}")

    paths: List[str] = []
    seen: Set[str] = set()
    for line_number, line in enumerate(
        config_path.read_text(encoding="utf-8").splitlines(), start=1
    ):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue

        path = normalize_sparse_path(stripped, line_number)
        if path not in seen:
            paths.append(path)
            seen.add(path)

    if not paths:
        raise ValueError(f"No sparse paths are configured in: {config_path}")

    return paths


def ensure_git_repository(rtthread_dir: Path) -> None:
    if not rtthread_dir.is_dir():
        raise RuntimeError(f"RT-Thread directory does not exist: {rtthread_dir}")

    result = run_git(rtthread_dir, "rev-parse", "--is-inside-work-tree", capture=True)
    if result.stdout.strip() != "true":
        raise RuntimeError(f"Not a Git work tree: {rtthread_dir}")

    top_level = run_git(rtthread_dir, "rev-parse", "--show-toplevel", capture=True)
    top_level_path = Path(top_level.stdout.strip()).resolve()
    if os.path.normcase(str(top_level_path)) != os.path.normcase(str(rtthread_dir.resolve())):
        raise RuntimeError(
            "RT-Thread submodule is not initialized as an independent Git work tree: "
            f"{rtthread_dir}"
        )


def get_status(rtthread_dir: Path) -> str:
    return run_git(rtthread_dir, "status", "--porcelain", capture=True).stdout.strip()


def ensure_clean(rtthread_dir: Path) -> None:
    status = get_status(rtthread_dir)
    if status:
        raise RuntimeError(
            "RT-Thread work tree is not clean. Commit/stash/revert local changes first:\n"
            + status
        )


def show_config(config_path: Path, paths: List[str]) -> None:
    print(f"Config: {config_path}")
    print("Configured sparse paths:")
    for path in paths:
        print(f"  {path}")


def show_status(rtthread_dir: Path) -> None:
    head = run_git(rtthread_dir, "rev-parse", "HEAD", capture=True).stdout.strip()
    print(f"RT-Thread: {rtthread_dir}")
    print(f"HEAD:      {head}")

    status = get_status(rtthread_dir)
    print("Work tree: clean" if not status else "Work tree: DIRTY")
    if status:
        print(status)

    try:
        sparse = run_git(rtthread_dir, "sparse-checkout", "list", capture=True).stdout.strip()
    except subprocess.CalledProcessError:
        sparse = ""

    if sparse:
        print("Sparse checkout: enabled")
        for path in sparse.splitlines():
            print(f"  {path}")
    else:
        print("Sparse checkout: not enabled or no cone paths are configured")


def apply_sparse(rtthread_dir: Path, config_path: Path, paths: List[str]) -> None:
    ensure_clean(rtthread_dir)
    show_config(config_path, paths)

    run_git(rtthread_dir, "sparse-checkout", "init", "--cone")
    run_git(
        rtthread_dir,
        "sparse-checkout",
        "set",
        "--cone",
        "--stdin",
        input_text="\n".join(paths) + "\n",
    )

    if get_status(rtthread_dir):
        raise RuntimeError(
            "Sparse-checkout finished, but the RT-Thread work tree is no longer clean."
        )

    print("\nSparse-checkout applied successfully.")
    show_status(rtthread_dir)


def disable_sparse(rtthread_dir: Path) -> None:
    ensure_clean(rtthread_dir)
    run_git(rtthread_dir, "sparse-checkout", "disable")
    print("Sparse-checkout disabled; the full RT-Thread work tree is restored.")
    show_status(rtthread_dir)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Manage RT-Thread sparse-checkout for ART-PI."
    )
    parser.add_argument(
        "action",
        nargs="?",
        choices=("apply", "status", "show", "disable"),
        default="status",
        help="action to execute (default: status)",
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=DEFAULT_CONFIG,
        help=f"sparse path config file (default: {DEFAULT_CONFIG.name})",
    )
    parser.add_argument(
        "--rt-thread-dir",
        type=Path,
        default=None,
        help="override the RT-Thread submodule path",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    try:
        config_path = args.config.expanduser().resolve()

        if args.rt_thread_dir is not None:
            rtthread_dir = args.rt_thread_dir.expanduser().resolve()
        else:
            project_root = find_project_root(DEFAULT_RTTHREAD_RELATIVE)
            rtthread_dir = (project_root / DEFAULT_RTTHREAD_RELATIVE).resolve()

        ensure_git_repository(rtthread_dir)

        if args.action == "show":
            show_config(config_path, load_sparse_paths(config_path))
        elif args.action == "status":
            show_status(rtthread_dir)
        elif args.action == "apply":
            apply_sparse(rtthread_dir, config_path, load_sparse_paths(config_path))
        elif args.action == "disable":
            disable_sparse(rtthread_dir)
        else:
            raise AssertionError(f"Unsupported action: {args.action}")

        return 0
    except (FileNotFoundError, RuntimeError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
