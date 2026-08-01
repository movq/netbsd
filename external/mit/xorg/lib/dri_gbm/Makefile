#	$NetBSD$

# Mesa's DRI backend for the GBM backend loader.

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIBISMODULE=	yes
LIB=		dri_gbm
LIBSUBDIR=	gbm
SHLIB_MAJOR=	0

.PATH:		${X11SRCDIR.Mesa}/src/gbm/backends/dri
SRCS=		gbm_dri.c

CPPFLAGS+= \
	-I${X11SRCDIR.Mesa}/include \
	-I${X11SRCDIR.Mesa}/include/drm-uapi \
	-I${X11SRCDIR.Mesa}/src \
	-I${X11SRCDIR.Mesa}/../src \
	-I${X11SRCDIR.Mesa}/src/mesa \
	-I${X11SRCDIR.Mesa}/src/loader \
	-I${X11SRCDIR.Mesa}/src/gbm/main \
	-I${X11SRCDIR.Mesa}/src/gbm/backends/dri \
	-I${X11SRCDIR.Mesa}/src/gallium/include \
	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary \
	-I${X11SRCDIR.Mesa}/src/gallium/frontends/dri \
	-I${DESTDIR}${X11INCDIR}/libdrm \
	-I${X11SRCDIR.drm}

.include "${.CURDIR}/../libGL/mesa-ver.mk"
CPPFLAGS+=	-DPACKAGE_VERSION=\"${MESA_VER}\" \
		-DHAVE_LIBDRM -DHAVE_PTHREAD \
		-DHAVE_TIMESPEC_GET -DHAVE_STRUCT_TIMESPEC

CFLAGS+=	-fvisibility=hidden -fno-strict-aliasing -pthread
LDFLAGS+=	-pthread

.include "../libloader.mk"
.include "../mesa-private-libs.mk"

LIBDPLIBS=	gallium	${.CURDIR}/../gallium \
		gbm		${.CURDIR}/../libgbm${OLD_SUFFIX} \
		drm		${.CURDIR}/../libdrm \
		X11-xcb		${.CURDIR}/../libX11/libX11-xcb \
		xcb-dri2	${.CURDIR}/../libxcb/dri2 \
		xcb-randr	${.CURDIR}/../libxcb/randr \
		xcb		${.CURDIR}/../libxcb/libxcb \
		expat		${NETBSDSRCDIR}/external/mit/expat/lib/libexpat \
		terminfo	${NETBSDSRCDIR}/lib/libterminfo \
		execinfo	${NETBSDSRCDIR}/lib/libexecinfo \
		elf		${NETBSDSRCDIR}/external/bsd/elftoolchain/lib/libelf \
		z		${NETBSDSRCDIR}/lib/libz \
		m		${NETBSDSRCDIR}/lib/libm \
		pthread		${NETBSDSRCDIR}/lib/libpthread

MKLINT=		no

.include <bsd.x11.mk>
.include <bsd.lib.mk>
