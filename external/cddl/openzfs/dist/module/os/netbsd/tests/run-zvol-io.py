#!/usr/bin/env python3
"""Exercise the native volume I/O adapter with a host DMU/device substitute."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix="openzfs-zvol-io-") as tmp:
    tmp = Path(tmp)
    (tmp / "sys").mkdir()
    for name in ("zfs_context", "dmu_objset", "dmu_tx", "dkio", "disklabel",
                 "spa_impl", "zil_impl", "zvol_impl_os", "zvol_os"):
        (tmp / "sys" / (name + ".h")).write_text('#include "zvol_io_shim.h"\n')
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get(
        "ZVOL_TEST_CFLAGS", "-fsanitize=address,undefined -fno-omit-frame-pointer"))
    subprocess.run(cc + ["-std=gnu11", "-g", "-O1", "-Wall", "-Wextra",
                        "-Werror", "-Wno-unused-parameter",
                        "-Wno-sign-compare", "-I" + str(tmp),
                        "-I" + str(tests)] + flags +
                   [str(tests.parent / "zfs/zvol_io.c"),
                    str(tests / "zvol_io_test.c"),
                    "-o", str(tmp / "zvol-io-test")], check=True)
    subprocess.run([str(tmp / "zvol-io-test")], check=True, timeout=30)
