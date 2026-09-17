#!/usr/bin/env python3
"""Test the private cache adapter with a host substitute for native pool_cache."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
zfs = tests.parents[3]
with tempfile.TemporaryDirectory(prefix="openzfs-cache-") as tmp:
    tmp = Path(tmp)
    (tmp / "sys/sys").mkdir(parents=True)
    for name in ("param", "atomic", "debug", "pool", "vmem", "systm", "sys/kmem"):
        (tmp / "sys" / (name + ".h")).write_text('#include "cache_shim.h"\n')
    (tmp / "sys/kmem.h").symlink_to(zfs / "include/os/netbsd/spl/sys/kmem.h")
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get(
        "CACHE_TEST_CFLAGS", "-fsanitize=address,undefined -fno-omit-frame-pointer"))
    command = cc + ["-std=gnu11", "-g", "-O1", "-pthread", "-Wall", "-Wextra",
                    "-Werror", "-Wno-unused-parameter", "-Wno-sign-compare",
                    "-I" + str(tmp), "-I" + str(tests),
                    "-include", str(tests / "cache_shim.h")] + flags
    command += [str(zfs / "module/os/netbsd/spl/spl_kmem_cache.c"),
                str(tests / "taskq_shim.c"), str(tests / "cache_test.c"),
                "-o", str(tmp / "cache-test")]
    subprocess.run(command, check=True)
    subprocess.run([str(tmp / "cache-test")], check=True, timeout=30)
