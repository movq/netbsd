# NetBSD OpenZFS userspace build

## Build organization

The libraries are private archives built from this import: libspl, libavl,
libnvpair, libuutil, libtpool, libzfs_core, libzutil, libshare, and libzfs.
The commands link those archives directly, plus the native crypto, zlib,
math, util, pthread, gettext, and C libraries. They do not link the installed
osnet ZFS libraries. OpenZFS's administrative commands do not need libzpool.

The base-system build selects this tree with `MKZFS`. The imported sources
live in `dist`, with NetBSD build files and `zfs_config.h` alongside it.
The kernel module build in `sys/modules/zfs` also uses `dist`.

Only `zfs`, `zpool`, their main manual pages, and private libraries are built
here. Public libraries and headers, additional manual pages, `mount_zfs`,
`zdb`, `ztest`, rump support and its tests, ZFS support in `fstyp`, `fstat`,
and `cgdconfig`, and the ZFS-root ramdisk are deferred. The old osnet ZFS
targets are no longer part of the base-system build; osnet still supplies
DTrace, CTF, and kernel Solaris compatibility.

Platform sources live in the respective `os/netbsd` directories under `dist`.
Userspace SPL headers live in `dist/lib/libspl/include/os/netbsd`; kernel
SPL headers are not used for this build. `zfs_config.h` records the native
build configuration without running upstream's Linux/FreeBSD configure
machinery.

## Native interfaces

* The ioctl adapter uses the kernel's shared version-15 indirect envelope.
  The driver copies command results back even when an ioctl returns an error,
  including the buffer-size update needed for nvlist retries. There is no
  translation to the old osnet ioctl ABI.
* Pool discovery retains osnet's directory scanning, block-device enumeration,
  preference for raw-device reads, and exclusion of parent disks with wedges.
  Device sizes and cache flushes use native disk ioctls. Disks and wedges are
  supplied already partitioned; automatic disk partitioning is not provided.
* Mount-table access uses `getvfsstat` and `statvfs`, with per-thread storage.
  Mount and unmount use native calls and the kernel's retained osnet mount
  argument layout. The kernel's existing restrictions on mount updates still
  apply; lazy unmount is unsupported.
* Module loading and host IDs follow the old native integration. The module
  exports `vfs.zfs.version.module` for the version commands.
* NFS sharing uses `/etc/zfs/exports`, the common OpenZFS export-file helpers,
  and SIGHUP to mountd, following the old integration's policy. SMB sharing
  remains unsupported.

The kernel bring-up limitations still apply, including unsupported encryption,
channel programs, and local use of long filenames.
