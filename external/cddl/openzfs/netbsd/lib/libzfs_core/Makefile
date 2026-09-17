# $NetBSD$

.include <bsd.init.mk>
LIB=	zfs_core
SRCS=	libzfs_core.c libzfs_core_ioctl.c
CPPFLAGS+=	-I${ZFSDIR}/lib/libzfs_core
.PATH:	${ZFSDIR}/lib/libzfs_core
.PATH:	${ZFSDIR}/lib/libzfs_core/os/netbsd
.include <bsd.lib.mk>
