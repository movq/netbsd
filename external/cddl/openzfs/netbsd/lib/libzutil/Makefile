# $NetBSD$

.include <bsd.init.mk>
LIB=	zutil
SRCS=	zutil_device_path.c zutil_import.c zutil_nicenum.c zutil_pool.c
SRCS+=	zutil_device_path_os.c zutil_import_os.c
CPPFLAGS+=	-I${ZFSDIR}/lib/libzutil
.PATH:	${ZFSDIR}/lib/libzutil ${ZFSDIR}/lib/libzutil/os/netbsd
.include <bsd.lib.mk>
