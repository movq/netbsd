#!/usr/bin/env python3
"""Check native rename lock/reference cleanup with host vnode substitutes."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
text = (tests.parent / "zfs/zfs_vnops_os.c").read_text()
start = text.index("\nint\nzfs_rename(")
source = text[start:text.index("\n}\n", start) + 3]
with tempfile.TemporaryDirectory(prefix="openzfs-rename-") as tmp:
    tmp = Path(tmp)
    (tmp / "rename.h").write_text(source)
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get(
        "ZFS_TEST_CFLAGS", "-fsanitize=address,undefined -fno-omit-frame-pointer"))
    subprocess.run(cc + ["-std=gnu11", "-g", "-O1", "-Wall", "-Wextra",
                        "-Werror", "-Wno-unused-parameter",
                        "-I" + str(tmp)] + flags +
                   [str(tests / "rename_test.c"), "-o", str(tmp / "test")],
                   check=True)
    subprocess.run([str(tmp / "test")], check=True, timeout=30)
