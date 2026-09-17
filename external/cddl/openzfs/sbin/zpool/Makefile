# $NetBSD$
.include <bsd.init.mk>
PROG=	zpool
MAN=	zpool.8 zpool-features.7
SRCS=	zpool_main.c zpool_iter.c zpool_util.c zpool_vdev.c zpool_vdev_os.c
CPPFLAGS+=	-I${ZFSDIR}/cmd/zpool
.PATH:	${ZFSDIR}/cmd/zpool ${ZFSDIR}/cmd/zpool/os/netbsd
.include <bsd.prog.mk>
