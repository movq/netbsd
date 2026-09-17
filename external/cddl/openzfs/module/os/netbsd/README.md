# NetBSD kernel integration

Work in progress; compile individual objects through `sys/modules/zfs`.
Use a fresh object directory to avoid reusing objects from the old ZFS tree.
The `osnet` headers are a temporary SPL fallback; do not add its old ZFS
headers to the include path.

Pass `MAKEOBJDIR=/absolute/path` on the make wrapper's command line to use
an isolated object directory; the wrapper overrides the environment setting.
Run `depend` after adding sources, then request individual `.o` targets.
The current source list has 191 objects, all cross-compiled for amd64.
No module link has been attempted.

Native disk vdevs use NetBSD buffer I/O and a workqueue for cache flushes.
ABD buffers are returned from the ZIO taskq, outside interrupt context.
Disk TRIM and file hole punching are unsupported. ARC memory accounting
and pageout throttling use the old integration's UVM and KVA policies.
The file adapter holds native file references for descriptor-based I/O
and uses uninstalled file objects for path-based I/O.

The native control device, VFS/vnode operations, zvol attachment, and module
lifecycle still need integration. Before implementing the control ioctl
adapter, decide the userspace compatibility scope: osnet accepts ABI 7 and
older layouts, while upstream FreeBSD OpenZFS accepts ABIs 15 and 7.
Keeping the existing versioned `zfs_iocparm_t` wrapper is possible, but the
old command numbers and `zfs_cmd_t` layouts need explicit translation.

Encryption is deliberately unsupported during bring-up. The NetBSD
`zio_crypt.c` returns errors or panics if encryption operations are attempted.
Page-backed direct I/O is disabled; requests use the upstream ARC fallback.

Channel programs are unsupported (`ENOTSUP`); the module excludes Lua.
Bulk snapshot deletion uses a C sync task. Userspace commands that depend
on channel programs will need adaptation.
