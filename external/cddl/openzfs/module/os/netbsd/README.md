# NetBSD kernel integration

Work in progress; compile individual objects through `sys/modules/zfs`.
Use a fresh object directory to avoid reusing objects from the old ZFS tree.
The `osnet` headers are a temporary SPL fallback; do not add its old ZFS
headers to the include path.

Encryption is deliberately unsupported during bring-up. The NetBSD
`zio_crypt.c` returns errors or panics if encryption operations are attempted.
Page-backed direct I/O is disabled; requests use the upstream ARC fallback.
