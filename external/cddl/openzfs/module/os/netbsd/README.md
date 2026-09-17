# NetBSD kernel integration

Work in progress; compile individual objects through `sys/modules/zfs`.
Use a fresh object directory to avoid reusing objects from the old ZFS tree.
The `osnet` headers are a temporary SPL fallback; do not add its old ZFS
headers to the include path.

Pass `MAKEOBJDIR=/absolute/path` on the make wrapper's command line to use
an isolated object directory; the wrapper overrides the environment setting.
Run `depend` after adding sources, then request individual `.o` targets.
The current source list has 203 objects, all cross-compiled for amd64.
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
VFS/mount operations, vnode operations (including paging), and the control
directory still need integration. Module loading and native concurrency
have not been tested.

The control device requires new OpenZFS binaries; there is
no compatibility with osnet command numbers or layouts. Its indirect ioctl
envelope is defined in `sys/zfs_ioctl_os.h`, uses ABI version 15 and the
current `zfs_cmd_t` size, and returns command data even on ioctl errors.
Porting userland is deferred until the kernel module builds.

The filename-length policy needs a decision before completing the
filesystem interfaces. OpenZFS's `longname` feature permits 1,023-byte
names; NetBSD's `KERNEL_NAME_MAX` restricts pathname lookup to 255 bytes,
and native `struct dirent` has a 512-byte name array. Setting `longname=off`
does not remove existing long names from an imported/received dataset.
No feature gate or mount policy has been implemented yet. Options include
rejecting affected dataset mounts while retaining pool/send/receive support,
disabling the pool feature entirely, or expanding NetBSD's native filename
interfaces. This requires agreement with the user.

Encryption is deliberately unsupported during bring-up. The NetBSD
`zio_crypt.c` returns errors or panics if encryption operations are attempted.
Page-backed direct I/O is disabled; requests use the upstream ARC fallback.

Channel programs are unsupported (`ENOTSUP`); the module excludes Lua.
Bulk snapshot deletion uses a C sync task. Userspace commands that depend
on channel programs will need adaptation.
