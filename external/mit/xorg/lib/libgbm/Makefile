#	$NetBSD: Makefile,v 1.11 2026/07/05 08:42:40 mrg Exp $

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIB=		gbm

.PATH:	${X11SRCDIR.Mesa}/src/gbm/main
SRCS.main=	\
	backend.c \
	gbm.c

SRCS=	${SRCS.main}

CPPFLAGS+=	-D_OS_UNIX=1
CPPFLAGS+=	-DHAVE_PTHREAD=1 -DHAVE_TIMESPEC_GET -DHAVE_STRUCT_TIMESPEC
CPPFLAGS+=	-DHAVE_NOATEXIT
CPPFLAGS+=	-DDEFAULT_BACKENDS_PATH=\"${X11USRLIBDIR}/gbm\"
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/include
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/gallium/include
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/loader
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/gbm/main
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/mapi
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${X11SRCDIR.drm}
CFLAGS+=	-fno-strict-aliasing -fexceptions

COPTS.gbm_dri.c+=	-Wno-error

.include "../libloader.mk"
.include "../mesa-private-libs.mk"

.PATH:	${X11SRCDIR.Mesa}/src/gbm/main
INCS=		gbm.h gbm_backend_abi.h
INCSDIR=	${X11INCDIR}

LIBDPLIBS=	drm     ${.CURDIR}/../libdrm
LIBDPLIBS+=	expat	${NETBSDSRCDIR}/external/mit/expat/lib/libexpat
LIBDPLIBS+=	pthread	${NETBSDSRCDIR}/lib/libpthread
LIBDPLIBS+=	m	${NETBSDSRCDIR}/lib/libm

PKGCONFIG=	gbm
PKGDIST.gbm=	${X11SRCDIR.Mesa}/../src/pkgconfig
.include "${.CURDIR}/../libGL/mesa-ver.mk"
CPPFLAGS+=	-DPACKAGE_VERSION=\"${MESA_VER}\"
PKGCONFIG_VERSION.gbm=	${MESA_VER}

.include "../libdrm/Makefile.defines"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
