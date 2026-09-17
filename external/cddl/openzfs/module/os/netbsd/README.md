# NetBSD kernel integration

Work in progress; compile individual objects through `sys/modules/zfs`.
Use a fresh object directory to avoid reusing objects from the old ZFS tree.
The `osnet` headers are a temporary SPL fallback; do not add its old ZFS
headers to the include path.

Pass `MAKEOBJDIR=/absolute/path` on the make wrapper's command line to use
an isolated object directory; the wrapper overrides the environment setting.
Run `depend` after adding sources, then request individual `.o` targets.
The current source list has 205 objects, all cross-compiled for amd64.
No module link has been attempted.

Native disk vdevs use NetBSD buffer I/O and a workqueue for cache flushes.
ABD buffers are returned from the ZIO taskq, outside interrupt context.
Disk TRIM and file hole punching are unsupported. ARC memory accounting
and pageout throttling use the old integration's UVM and KVA policies.
The file adapter holds native file references for descriptor-based I/O
and uses uninstalled file objects for path-based I/O.

The control device, zvol attachment and I/O, and module lifecycle compile.
Volume strategy I/O is synchronous, as in osnet; block and raw device opens
retain NetBSD's final-close accounting. Pool hooks and LWP I/O accounting
are integrated, and the debug log uses the portable FreeBSD implementation.

Znode metadata, ACLs, and directory routines use current OpenZFS code from
the FreeBSD directory with NetBSD branches. Native `zfs_vcache.c` supplies
vcache construction and lookup, genfs initialization, and UVM size updates.
Extended-attribute directory deletion retains osnet's synchronous handling.
The mount adapter retains native mount arguments, vcache, filesystem
suspension, and unmount flushing. Current OpenZFS dataset setup, quotas,
and property callbacks are shared with the FreeBSD source. Filesystem
initialization belongs to the module lifecycle, avoiding duplicate calls
from VFS attach/detach.

Vnode operations (including paging), filehandle operations, and the control
directory still need integration. Module loading and native concurrency
have not been tested.

The control device requires new OpenZFS binaries; there is
no compatibility with osnet command numbers or layouts. Its indirect ioctl
envelope is defined in `sys/zfs_ioctl_os.h`, uses ABI version 15 and the
current `zfs_cmd_t` size, and returns command data even on ioctl errors.
Porting userland is deferred until the kernel module builds.

The agreed filename-length policy retains NetBSD's native limits.
OpenZFS's `longname` feature permits 1,023-byte
names; NetBSD's `KERNEL_NAME_MAX` restricts pathname lookup to 255 bytes,
and native `struct dirent` has a 512-byte name array. Setting `longname=off`
does not remove existing long names from an imported/received dataset.
Local `longname=on` requests return `ENOTSUP`. Mount and resume reject
datasets (including snapshots) with active longname feature state.
Administrative holds, pool import, and send/receive retain feature support.
If an online receive or rollback introduces longname use, resume fails and
requests forced unmount when the ioctl releases its filesystem reference.
The host test `tests/run-vfs-policy.py` exercises the actual property arm,
mount predicate, resume function, and VFS release under ASan/UBSan; it does
not simulate native vnode or suspension concurrency.

Encryption is deliberately unsupported during bring-up. The NetBSD
`zio_crypt.c` returns errors or panics if encryption operations are attempted.
Page-backed direct I/O is disabled; requests use the upstream ARC fallback.

Channel programs are unsupported (`ENOTSUP`); the module excludes Lua.
Bulk snapshot deletion uses a C sync task. Userspace commands that depend
on channel programs will need adaptation.
