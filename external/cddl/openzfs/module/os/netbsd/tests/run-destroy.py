#!/usr/bin/env python3
"""Test the C snapshot batch adapter with simulated DSL operations."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
zfs = tests.parents[3]
with tempfile.TemporaryDirectory(prefix="openzfs-destroy-") as tmp:
    tmp = Path(tmp)
    (tmp / "sys").mkdir()
    for name in ("zfs_context", "dmu_tx", "dsl_dir", "dsl_destroy",
                 "dsl_synctask", "nvpair"):
        (tmp / "sys" / (name + ".h")).write_text('#include "destroy_shim.h"\n')
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get(
        "DESTROY_TEST_CFLAGS",
        "-fsanitize=address,undefined -fno-omit-frame-pointer"))
    command = cc + ["-std=gnu11", "-g", "-O1", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(tmp), "-I" + str(tests)] + flags
    command += [str(zfs / "module/os/netbsd/zfs/dsl_destroy_os.c"),
                str(tests / "destroy_test.c"), "-o", str(tmp / "destroy-test")]
    subprocess.run(command, check=True)
    subprocess.run([str(tmp / "destroy-test")], check=True, timeout=30)
