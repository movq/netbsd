# $NetBSD$

.include <bsd.own.mk>

ZFSDIR=	${NETBSDSRCDIR}/external/cddl/openzfs
ZFSNETBSD=	${ZFSDIR}/netbsd

# Keep the bring-up libraries private until the remaining osnet consumers
# have been converted. In particular, do not mix the two libnvpair ABIs.
LIBISPRIVATE=	yes
MKPIC=		no
MKPROFILE=	no
MKLINT=		no
WARNS=		0

CPPFLAGS+=	-include ${ZFSNETBSD}/zfs_config.h
CPPFLAGS+=	-I${ZFSDIR}/lib/libspl/include/os/netbsd
CPPFLAGS+=	-I${ZFSDIR}/lib/libspl/include
CPPFLAGS+=	-I${ZFSDIR}/include
CPPFLAGS+=	-I${ZFSDIR}/include/os/netbsd/zfs
CPPFLAGS+=	-DTEXT_DOMAIN=\"zfs\"
COPTS+=		-std=gnu99
