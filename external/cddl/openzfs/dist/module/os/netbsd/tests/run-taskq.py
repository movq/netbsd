#!/usr/bin/env python3
"""Compile the real taskq implementation with host pthread/callout substitutes."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
zfs = tests.parents[3]
with tempfile.TemporaryDirectory(prefix="openzfs-taskq-") as tmp:
    tmp = Path(tmp)
    (tmp / "sys").mkdir()
    # Only OS interfaces are substituted. Use the real taskq and AVL headers.
    for name in ("param", "atomic", "callout", "condvar", "debug", "kmem",
                 "mutex", "threadpool", "time", "tsd", "proc", "types", "stdint",
                 "cmn_err", "mod", "sysmacros"):
        (tmp / "sys" / (name + ".h")).write_text(
            '#include "taskq_shim.h"\n')
    (tmp / "sys/taskq.h").symlink_to(
        zfs / "include/os/netbsd/spl/sys/taskq.h")
    for name in ("avl", "avl_impl"):
        (tmp / "sys" / (name + ".h")).symlink_to(
            zfs / "include/sys" / (name + ".h"))
    # Host pthread headers still need the host's actual sys/types.h.
    (tmp / "sys/types.h").write_text("#include_next <sys/types.h>\n")
    # Compile the actual recurring-task functions as well. Their surrounding
    # pool/FMA machinery is supplied by the test, so a failed shutdown hangs
    # or leaves a live task just as it would in the kernel.
    callers = []
    for source, names in (
        ("spa_misc.c", ("spa_deadman", "spa_deadman_stop", "spa_deadman_start")),
        ("zfs_fm.c", ("zfs_ereport_schedule_cleaner", "zfs_ereport_taskq_fini")),
    ):
        text = (zfs / "module/zfs" / source).read_text()
        for name in names:
            match = re.search(
                r"^(?:static )?void\n" + name + r"\([^;]*?\n\{.*?^\}",
                text, re.MULTILINE | re.DOTALL)
            if match is None:
                raise RuntimeError("Cannot locate " + name)
            callers.append(match.group())
    (tmp / "taskq_callers.h").write_text("\n\n".join(callers) + "\n")
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get(
        "TASKQ_TEST_CFLAGS", "-fsanitize=address,undefined -fno-omit-frame-pointer"))
    command = cc + ["-std=gnu11", "-g", "-O1", "-pthread", "-Wall", "-Wextra",
                    "-Werror", "-Wno-unused-parameter", "-Wno-sign-compare",
                    "-I" + str(tmp), "-I" + str(tests),
                    "-include", str(tests / "taskq_shim.h")] + flags
    command += [str(zfs / "module/os/netbsd/spl/spl_taskq.c"),
                str(zfs / "module/avl/avl.c"),
                str(tests / "taskq_shim.c"), str(tests / "taskq_test.c"),
                "-o", str(tmp / "taskq-test")]
    subprocess.run(command, check=True)
    subprocess.run([str(tmp / "taskq-test")], check=True, timeout=60)
