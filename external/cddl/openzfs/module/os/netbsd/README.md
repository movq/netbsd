# NetBSD kernel integration

This port is being brought up against OpenZFS 2.4.4, one compilation unit at
a time. A complete kernel module is not yet available.

The source layout follows the other OpenZFS ports:

* `module/os/netbsd/spl`: NetBSD implementations of portability interfaces.
* `module/os/netbsd/zfs`: VM, VFS, vnode, device and module integration.
* `include/os/netbsd/spl` and `include/os/netbsd/zfs`: corresponding headers.

`sys/modules/zfs/Makefile.zfsmod` now selects OpenZFS common sources. Its
source inventory still needs the OS implementations, compression and crypto
libraries, and Lua. The old `external/cddl/osnet` headers are a temporary
fallback for the existing NetBSD Solaris compatibility layer. Do not add the
old ZFS source or header directories to the search path: their interfaces and
on-disk definitions do not match the new core.

Use a fresh object directory, since an earlier NetBSD build can contain
objects with the same names compiled from the old ZFS tree. For example,
from `sys/modules/zfs`, using the toolchain from the initial build:

```
mkdir -p ~/obj/netbsd/zfs-openzfs-module
nbmake-amd64 MAKEOBJDIR=~/obj/netbsd/zfs-openzfs-module obj
nbmake-amd64 MAKEOBJDIR=~/obj/netbsd/zfs-openzfs-module cityhash.o
```

The wrapper must be on `PATH`, or invoked by its absolute path. Compile
individual object targets during bring-up; defer linking until the source
inventory and OS interfaces are complete. The rump build also includes
`Makefile.zfsmod` and will need verification after kernel integration.

Compiled so far with the amd64 kernel toolchain and `-Werror`:
`cityhash.o`, `zfs_valstr.o`, `objlist.o`, `aggsum.o`, and `btree.o`.
This checks compilation only, not runtime behavior or module symbol
resolution. The context adapters still use the old allocation and locking
implementation. In particular, the old condition-variable timeout and signal
return conventions need review before compiling their new callers. The UIO
adapter currently covers only the old buffered-I/O representation.
