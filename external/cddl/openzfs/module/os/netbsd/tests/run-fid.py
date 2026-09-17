#!/usr/bin/env python3
"""Check the real filehandle adapter with exact-sized host buffers."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
zfs = tests.parents[3]
formats = (zfs / "include/os/netbsd/zfs/sys/zfs_vfsops_os.h").read_text()
formats = formats[formats.index("typedef struct zfid_short {"):
                  formats.index("extern int zfs_super_owner;")]
with tempfile.TemporaryDirectory(prefix="openzfs-fid-") as tmp:
    tmp = Path(tmp)
    (tmp / "fid_format.h").write_text(formats)
    (tmp / "sys").mkdir()
    for name in ("zfs_context", "zfs_ctldir", "zfs_ioctl_impl", "dmu_objset"):
        (tmp / "sys" / (name + ".h")).write_text('#include "fid_shim.h"\n')
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get(
        "ZFS_TEST_CFLAGS", "-fsanitize=address,undefined -fno-omit-frame-pointer"))
    subprocess.run(cc + ["-std=gnu11", "-g", "-O1", "-Wall", "-Wextra",
                        "-Werror", "-Wno-unused-parameter",
                        "-I" + str(tmp), "-I" + str(tests)] + flags +
                   [str(tests.parent / "zfs/zfs_fid_os.c"),
                    str(tests / "fid_test.c"), "-o", str(tmp / "fid-test")],
                   check=True)
    subprocess.run([str(tmp / "fid-test")], check=True, timeout=30)
