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

## Next work and architectural decision

The first attempt at `zfs_deleg.o` reached the missing `sys/abd_os.h`.
It needs the OS buffer representation before the DMU headers can compile.
The FreeBSD ABD implementation is a useful reference for NetBSD; its VM page
mapping and direct-I/O portions cannot be copied unchanged.

Before extending the DMU and encrypted-I/O integration, choose the native
encryption backend. The old NetBSD ZFS port predates this feature:

* OpenZFS's bundled Illumos Crypto Provider (ICP) supplies AES-GCM and AES-CCM.
  The implementation in `module/os/linux/zfs/zio_crypt.c` uses that API, as does
  the default branch of `include/sys/zio_crypt.h`. Using it requires porting
  ICP's kernel dependencies and initialization, but retains the upstream
  encryption implementation.
* FreeBSD uses its native opencrypto framework through `crypto_os.c` and a
  separate `zio_crypt.c`. NetBSD has AES-GCM in `sys/opencrypto`, but no
  AES-CCM algorithm in its current opencrypto interface. That route therefore
  requires more than adapting the FreeBSD session/request API if we are to
  support all existing OpenZFS encrypted datasets.

The proposed starting point is ICP, initially using portable implementations,
with acceleration considered after correctness and kernel FPU handling have
been established. This is a proposal, not an implemented backend selection.

Other integration areas to track:

* Modern nvpair, AVL, list and Unicode code overlaps symbols currently supplied
  by the `solaris` module, which is also used by DTrace. Settle ownership and
  compatibility before adding those libraries to the final module.
* Port ARC memory accounting/reclaim and ABD allocation, then task queues and
  their newer interfaces.
* Adapt NetBSD vdev, vnode/VFS, zvol, ioctl and module lifecycle code against
  the new core interfaces.
* Supply events, statistics, module parameters, compression libraries and Lua.
* Review the buffered UIO adapter when adding the OS vnode code; it does not
  yet implement OpenZFS's direct-I/O state or page-pinning interfaces.

No module link has been attempted.
