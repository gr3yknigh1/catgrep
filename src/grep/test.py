#!/usr/bin/python3
from __future__ import annotations
from typing import TYPE_CHECKING, Sequence, cast

if TYPE_CHECKING:
    from _typeshed import StrPath

import sys

if sys.version_info < (3, 7):
    raise Exception("python>=3.7 is required")

import os
import os.path
import shlex
import shutil
import argparse
import subprocess
import logging


logging.basicConfig(
    level=os.getenv("LOG_LEVEL", "INFO"),
    format="%(message)s",
)

logger = logging.getLogger("test")



GREP_BIN = shutil.which("grep")

if GREP_BIN is None:
    raise FileNotFoundError("Unable to find cat binary in PATH")


def _raise_if_not_exists(path: StrPath):
    if not os.path.exists(path):
        raise FileNotFoundError(f"Unable to find file with given path: {path!r}")


def compare_proc_output(exec_a: StrPath, exec_b: StrPath, flags: Sequence[str]) -> bool:
    template = "{exec} {flags}"

    proc_a = subprocess.run(
        shlex.split(
            template.format(
                exec=exec_a,
                flags=" ".join(flags),
            ),
        ),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    proc_b = subprocess.run(
        shlex.split(
            template.format(
                exec=exec_b,
                flags=" ".join(flags),
            ),
        ),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    logger.debug("stdout")
    logger.debug(f"A: {proc_a.stdout!s}")
    logger.debug(f"B: {proc_b.stdout!s}")
    logger.debug("stderr")
    logger.debug(f"A: {proc_a.stderr!s}")
    logger.debug(f"B: {proc_b.stderr!s}")

    return proc_a.stdout == proc_b.stdout and proc_a.stderr == proc_b.stderr and proc_a.returncode == proc_b.returncode

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("test_bin")
    parser.add_argument("test_flags")
    args = parser.parse_args()
    test_bin: str = args.test_bin
    test_flags: str = args.test_flags

    _raise_if_not_exists(test_bin)
    _raise_if_not_exists(test_flags)

    with open(test_flags) as f:
        flag_packs = [shlex.split(line) for line in f.read().split('\n')]
        flag_packs = filter(lambda pack: len(pack) > 0, flag_packs)

    failed_packs = []

    logger.debug(f"flags: {flag_packs}")

    for index, flag_pack in enumerate(flag_packs):
        if not compare_proc_output(cast(str, GREP_BIN), test_bin, flag_pack):
            failed_packs.append(flag_pack)
            logger.error(f"[{index+1:3}] FAILED {flag_pack!r}")
        else:
            logger.info(f"[{index+1:3}] PASSED {flag_pack!r}")

    for flag_pack in failed_packs:
        logger.info(f"FAILED: {(' '.join(flag_pack))!r}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

