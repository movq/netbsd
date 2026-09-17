# $NetBSD$
.include <bsd.init.mk>
LIB=	share
SRCS=	libshare.c nfs.c nfs_os.c smb_os.c
CPPFLAGS+=	-I${ZFSDIR}/lib/libshare
.PATH:	${ZFSDIR}/lib/libshare ${ZFSDIR}/lib/libshare/os/netbsd
.include <bsd.lib.mk>
