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
import itertools
import logging


logging.basicConfig(
    level=os.getenv("LOG_LEVEL", "INFO")
)

logger = logging.getLogger("test")



CAT_BIN = shutil.which("cat")

if CAT_BIN is None:
    raise FileNotFoundError("Unable to find cat binary in PATH")


FLAGS = [
    "",
    "-b",
    "-n",
    "-s",
    "-v",
    "-T",
    "-E",
    "-A",
]

flags_combinations = set()

for i in range(len(FLAGS)):
    flags_combinations.update(itertools.combinations(FLAGS, i))


def compare_proc_output(exec_a: StrPath, exec_b: StrPath, flags: Sequence[str], test_file: StrPath) -> bool:
    template = "{exec} {flags} {file_path}"

    proc_a = subprocess.run(
        shlex.split(
            template.format(
                exec=exec_a,
                flags=" ".join(flags),
                file_path=shlex.quote(cast(str, test_file)),
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
                file_path=shlex.quote(cast(str, test_file)),
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

    return proc_a.stdout == proc_b.stdout and proc_a.stderr == proc_b.stderr

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("test_bin")
    parser.add_argument("test_files", nargs="*")
    args = parser.parse_args()
    test_bin: str = args.test_bin
    test_files: Sequence[str] = args.test_files

    if not os.path.exists(test_bin):
        raise FileNotFoundError(f"Unable to find bin with given path: {test_bin!r}")

    failed = []

    for index, flags in enumerate(flags_combinations):

        for test_file in test_files:

            if not os.path.exists(test_file):
                raise FileNotFoundError(f"Test file doesn't exists: file={test_file!r}")

            if not os.path.isfile(test_file):
                raise Exception(f"Test file isn't a file: file={test_file!r}")

            if not compare_proc_output(cast(str, CAT_BIN), test_bin, flags, test_file):
                failed.append((test_file, flags))
                logger.error(f"[{index+1:3}] FAILED")
            else:
                logger.info(f"[{index+1:3}] PASSED {flags!r}")

    for (test_file, flags) in failed:
        logger.info(f"FAILED: {test_file!r} {flags!r}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

