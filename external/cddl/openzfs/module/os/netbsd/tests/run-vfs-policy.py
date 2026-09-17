#!/usr/bin/env python3
"""Run the actual mount-policy, resume and VFS-release functions on the host."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
native = tests.parent / "zfs"
common = tests.parents[2] / "zfs/zfs_ioctl.c"


def function(path, name):
    text = path.read_text()
    match = re.search(r"\n(?:int|void)\n" + name + r"\(", text)
    if not match:
        raise ValueError(f"missing function {name}")
    end = text.index("\n}\n", match.start()) + 3
    return text[match.start():end]


source = '#include "vfs_policy_shim.h"\n#define __NetBSD__ 1\n'
source += function(native / "zfs_vfsops_os.c", "zfs_netbsd_check_mount")
source += function(native / "zfs_vfsops_os.c", "zfs_resume_fs")
source += function(native / "zfs_ioctl_os.c", "zfs_vfs_rele")

# Compile the real longname arm of the ioctl property setter. The surrounding
# switch handles unrelated properties and requires most of the kernel module.
ioctl = common.read_text()
arm = ioctl[ioctl.index("\tcase ZFS_PROP_LONGNAME:\n"):]
arm = arm[:arm.index("\tcase ZFS_PROP_DEFAULTUSERQUOTA:\n")]
source += """
int set_longname(int source, uint64_t intval)
{
    const char *dsname = "pool/fs";
    int err = 0;
    switch (ZFS_PROP_LONGNAME) {
""" + arm + """
    }
    return err;
}
"""

with tempfile.TemporaryDirectory(prefix="openzfs-vfs-policy-") as tmp:
    tmp = Path(tmp)
    (tmp / "adapter.c").write_text(source)
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get(
        "ZFS_TEST_CFLAGS", "-fsanitize=address,undefined -fno-omit-frame-pointer"))
    subprocess.run(cc + ["-std=gnu11", "-g", "-O1", "-Wall", "-Wextra",
                        "-Werror", "-Wno-unused-parameter",
                        "-I" + str(tests)] + flags +
                   [str(tmp / "adapter.c"), str(tests / "vfs_policy_test.c"),
                    "-o", str(tmp / "vfs-policy-test")], check=True)
    subprocess.run([str(tmp / "vfs-policy-test")], check=True, timeout=30)
