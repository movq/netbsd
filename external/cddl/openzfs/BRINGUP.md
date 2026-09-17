# NetBSD OpenZFS runtime bringup

## Verified configuration

The initial runtime smoke tests used an amd64 GENERIC kernel reporting
NetBSD 11.99.8, built from this tree, and OpenZFS 2.4.4. The test VM had
eight CPUs and four disposable 10 GiB SCSI disks. The kernel, module, and
both commands were installed from the same build object tree.

Both `zfs version` and `zpool version` report `zfs-2.4.4-netbsd`; the
kernel version line is `zfs-kmod-zfs-2.4.4-netbsd`.

This is a runtime smoke milestone, not a full ZFS test-suite or durability
qualification. The unsupported features listed in README.md remain
unsupported.

## Problems found and fixed

* The kernel's BSD libnv and OpenZFS nvpair export overlapping names with
  incompatible interfaces. Private SPL header mappings keep the module's
  implementation separate.
* The retained Solaris module also exports nvpair allocator and fault
  management symbols. These need private names even if the first ZFS
  load succeeds: loading ZFS again with Solaris already present exposed
  the collisions.
* The Solaris AVL implementation uses a larger `avl_tree_t` than OpenZFS.
  Its `avl_create()` overwrote an adjacent mutex, causing pool creation to
  panic in `spa_spare_exists()`. The module now builds the matching
  OpenZFS AVL implementation under private names.
* Interior vdevs have NULL paths. The native zvol check must return false
  before attempting pathname lookup.
* The module must initialize its private task queues and kstat subsystem
  before ZFS initialization, and finalize them on unload and initialization
  failure. Otherwise the first syncing transaction group dereferences the
  NULL delayed-task queue.
* Pool cache statistics must use adaptive mutex operations for the
  `IPL_NONE` pool lock. Spin operations allowed the statistics reader to
  corrupt another thread's lock ownership during allocation.
* Native vnode reference counts contain flag bits; use `vrefcnt()` in
  assertions. Also, `zhold()` must use `vref()` to match `zrele()`'s
  `vrele()`, rather than taking a vnode hold count.
* Pool discovery must preserve osnet's `hw.disknames` enumeration for
  `/dev`. Scanning every device node concurrently triggered a native
  `vndopen()` panic while probing unconfigured vnd partitions. Restoring
  enumeration avoids that path; it does not fix the underlying vnd driver
  race. Explicit device paths and other search directories still use the
  common scanner.
* Temporary `$import-...` pool kstats cannot be represented as native
  sysctl names. Skip those probe statistics; the real imported pool
  publishes its own statistics.

## Tests completed

All of the following passed with the fixes:

* Module load, explicit unload after exporting pools, reload while Solaris
  remains loaded, and command-triggered module autoload after reboot.
* Mirrored pool and dataset creation, zstd compression, regular-file and
  directory creation, reads and writes, hard and symbolic links, rename,
  chmod, unlink, and rmdir.
* Byte comparisons of a 32 MiB random file and the approximately 30 MiB
  kernel image.
* Shared writable mmap, `msync(MS_SYNC)`, `fsync`, reads after unmapping,
  visibility of `pwrite` through a mapping, and zero-filled extension after
  truncation. These checks passed on both single-disk and RAIDZ datasets,
  including after reboot.
* Snapshot, clone, rollback, full send to an FFS file, receive into a new
  dataset, and comparisons of received and cloned data.
* `sync=always` writes, degraded mirror writes, online/resilver, replacement
  with a third disk, attachment of a fourth disk, and detach.
* Three-disk RAIDZ1 creation, degraded reads and writes, online/resilver,
  and scrub.
* Raw zvol creation, 32 MiB write/read comparison, and destruction.
* Dataset unmount/remount, clone and received-dataset destruction, pool
  export/import with both explicit `/dev` scanning and default discovery.
* Normal reboot with mounted datasets, import after reboot, persistent-data
  comparisons, and scrubs of both pools with zero errors.
* Reading `kstat.zfs.misc.arcstats.size`.

The module and userspace commands were rebuilt with `nbmake-amd64`.
Defined global symbols in the module were checked against both GENERIC and
`solaris.kmod`; no collisions remained. The existing host-side cache and
taskq tests also passed. Final post-reboot imports and scrubs produced no
new ZFS console diagnostics.

Crash recovery was exercised incidentally during bringup, but controlled
power-loss/ZIL replay, sustained memory pressure, and the full upstream
test suite have not been qualified.

## Test VM handoff

The VM at `root@10.77.0.2` has the updated `/netbsd`, `/sbin/zfs`,
`/sbin/zpool`, and `/stand/amd64/11.99.8/modules/zfs/zfs.kmod`.
Original installed files are saved under `/root/zfs-bringup/original`.
`/boot.cfg` selects `com0,115200`, and `ddb.onpanic=1` is set in
`/etc/sysctl.conf`.

The final pools are:

| Pool | Devices | Mountpoints |
| --- | --- | --- |
| `zsmoke` | sd1, single disk after mirror detach tests | `/zsmoke`, `/zsmoke/data` |
| `zraid` | sd2, sd3, sd4, RAIDZ1 | `/zraid` |

Both pools are ONLINE with no known data errors. Pool names may be imported
with `zpool import zsmoke` and `zpool import zraid` after another reboot;
automatic pool import has not been configured.

Host-side build logs, smoke-test transcripts, the mmap test source, and the
serial transcript are under `/home/mike/obj/vm/zfs-bringup`. The guest's
`/root/zfs-bringup` contains reference data, the send stream, and the mmap
test program.
