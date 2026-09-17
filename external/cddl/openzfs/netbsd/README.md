# NetBSD OpenZFS userspace build

The OpenZFS 2.4.4 `zfs` and `zpool` commands and their supporting libraries
compile and link for NetBSD/amd64. This is a compile/link milestone only.
Neither command has been run against the new module.

From `external/cddl/openzfs/netbsd`, using the tools and destination directory
prepared by `build.sh`:

```sh
/home/mike/obj/netbsd/zfs/tooldir.Linux-7.2.5-gentoo-x86_64/bin/nbmake-amd64 obj
/home/mike/obj/netbsd/zfs/tooldir.Linux-7.2.5-gentoo-x86_64/bin/nbmake-amd64 -j16 dependall
```

Run `obj` separately before the parallel build, so every submake starts in
its object directory. An optional `clean` before `dependall` rebuilds all
objects and libraries.

For this workspace, the resulting programs are:

```
/home/mike/obj/netbsd/zfs/external/cddl/openzfs/netbsd/sbin/zfs/zfs
/home/mike/obj/netbsd/zfs/external/cddl/openzfs/netbsd/sbin/zpool/zpool
```

The clean build log is
`/home/mike/obj/netbsd/zfs/external/cddl/openzfs/netbsd/build.log`.

## Build organization

The libraries are private archives built from this import: libspl, libavl,
libnvpair, libuutil, libtpool, libzfs_core, libzutil, libshare, and libzfs.
The commands link those archives directly, plus the native crypto, zlib,
math, util, pthread, gettext, and C libraries. They do not link the installed
osnet ZFS libraries. OpenZFS's administrative commands do not need libzpool.

This explicit build does not replace the osnet targets in the base-system
build. Shared-library installation, public header installation, manual pages,
sets lists, and the `mount_zfs` compatibility entry point remain integration
work. In particular, osnet's `zdb` and `ztest` still use its old libzpool and
must not be linked against these new libraries.

Platform sources live in the respective `os/netbsd` directories. Userspace
SPL headers live in `lib/libspl/include/os/netbsd`; kernel SPL headers are
not used for this build. `zfs_config.h` records the native build configuration
without running upstream's Linux/FreeBSD configure machinery.

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
channel programs, and local use of long filenames. Compiling the userspace
commands does not establish runtime support for their full command set.
See `../module/os/netbsd/README.md`.

## Verification

The clean amd64 build compiled 76 source files and linked both commands without
compiler warnings. Both outputs are NetBSD ELF64 PIE executables using
`/libexec/ld.elf_so`; neither depends on an osnet shared library.

A separate link of the `zfs` objects with all nine archives under
`--whole-archive` also succeeded. This checks every library member, including
members that the normal commands do not extract. The resulting verification
artifact is `zfs-all-libraries` in the userspace object root.

The dependency files contain no osnet, Linux-port, or FreeBSD-port inputs.
CTF from an amd64 userspace ioctl object compiled with `DBG=-g` was compared
with the built native kernel object. Sizes and all member offsets match for
`zfs_iocparm` (24 bytes), `zfs_cmd` (4,528), `zfs_share` (32),
`dmu_objset_stats` (288), `drr_begin` (304), `zinject_record` (368), and
`zfs_stat` (40). These checks do not establish runtime ioctl behavior.

The native kernel module also rebuilt and linked after the common-source
adjustments and addition of the version sysctl. Its log is
`/home/mike/obj/netbsd/zfs-openzfs-continue/userland-link.log`.
