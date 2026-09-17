# $NetBSD$

.include <bsd.own.mk>

ZFSNETBSD=	${NETBSDSRCDIR}/external/cddl/openzfs
ZFSDIR=		${ZFSNETBSD}/dist

# Keep the libraries private until public library integration is ready.
# In particular, do not mix the OpenZFS and osnet libnvpair ABIs.
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
