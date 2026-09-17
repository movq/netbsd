#!/usr/bin/env python3
"""Exercise the actual control-directory readdir with bounded host buffers."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
source = (tests.parent / "zfs/zfs_ctldir.c").read_text()
source = source[source.index("static int\nsfs_readdir("):
                source.index("static int\nsfs_inactive(")]
with tempfile.TemporaryDirectory(prefix="openzfs-ctldir-") as tmp:
    tmp = Path(tmp)
    (tmp / "readdir.h").write_text(source)
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get(
        "ZFS_TEST_CFLAGS", "-fsanitize=address,undefined -fno-omit-frame-pointer"))
    subprocess.run(cc + ["-std=gnu11", "-g", "-O1", "-Wall", "-Wextra",
                        "-Werror", "-Wno-unused-parameter",
                        "-I" + str(tmp)] + flags +
                   [str(tests / "ctldir_test.c"), "-o", str(tmp / "test")],
                   check=True)
    subprocess.run([str(tmp / "test")], check=True, timeout=30)
