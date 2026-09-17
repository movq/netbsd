# $NetBSD$

.include <bsd.init.mk>
LIB=	zfs
SRCS=	libzfs_changelist.c libzfs_config.c libzfs_crypto.c
SRCS+=	libzfs_dataset.c libzfs_diff.c libzfs_import.c libzfs_iter.c
SRCS+=	libzfs_mount.c libzfs_pool.c libzfs_sendrecv.c libzfs_status.c
SRCS+=	libzfs_util.c
SRCS+=	cityhash.c zfeature_common.c zfs_comutil.c zfs_deleg.c
SRCS+=	zfs_fletcher.c zfs_fletcher_aarch64_neon.c zfs_fletcher_avx512.c
SRCS+=	zfs_fletcher_intel.c zfs_fletcher_sse.c zfs_fletcher_superscalar.c
SRCS+=	zfs_fletcher_superscalar4.c zfs_namecheck.c zfs_prop.c zfs_valstr.c
SRCS+=	zpool_prop.c zprop_common.c
CPPFLAGS+=	-I${ZFSDIR}/lib/libzfs
.PATH:	${ZFSDIR}/lib/libzfs ${ZFSDIR}/module/zcommon
.PATH:	${ZFSDIR}/lib/libzfs/os/netbsd
.include <bsd.lib.mk>
