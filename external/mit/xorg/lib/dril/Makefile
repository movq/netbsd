#	$NetBSD$

# Mesa compatibility module for Xorg's legacy DRI interface.

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIBISMODULE=	yes
LIB=		dril_dri
LIBSUBDIR=	modules/dri
SHLIB_MAJOR=	0

DRIDIR=		${X11USRLIBDIR}/modules/dri
DRIDEBUGDIR=	${DEBUGDIR}${DRIDIR}

.PATH:		${X11SRCDIR.Mesa}/src/gallium/targets/dril
SRCS=		dril_target.c
VERSION_MAP=	dri.sym.in

CPPFLAGS+= \
	-I${X11SRCDIR.Mesa}/include \
	-I${X11SRCDIR.Mesa}/include/drm-uapi \
	-I${X11SRCDIR.Mesa}/src \
	-I${X11SRCDIR.Mesa}/../src \
	-I${X11SRCDIR.Mesa}/src/mesa \
	-I${X11SRCDIR.Mesa}/src/gbm/main \
	-I${X11SRCDIR.Mesa}/src/gallium/include \
	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary \
	-I${X11SRCDIR.Mesa}/src/gallium/drivers \
	-I${X11SRCDIR.Mesa}/src/gallium/winsys

CPPFLAGS+=	-DHAVE_LIBGBM -DHAVE_PTHREAD \
		-DHAVE_TIMESPEC_GET -DHAVE_STRUCT_TIMESPEC

CFLAGS+=	-std=gnu11 -fvisibility=hidden -fno-strict-aliasing -pthread
LDFLAGS+=	-pthread -Wl,--build-id=sha1
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_CPU} == "aarch64"
LDFLAGS+=	-Wl,-z,defs
.endif

.include "../mesa-private-libs.mk"

LIBDPLIBS=	gbm		${.CURDIR}/../libgbm \
		m		${NETBSDSRCDIR}/lib/libm \
		pthread		${NETBSDSRCDIR}/lib/libpthread

# Keep this list in sync with the drivers built by ../gallium.
DRIVERS=	kms_swrast swrast
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE} == "evbarm"
DRIVERS+=	nouveau
. if ${MKLLVMRT} != "no"
DRIVERS+=	r600 r300 radeonsi
. endif
.endif
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
DRIVERS+=	iris crocus
. if ${MKLLVMRT} != "no"
DRIVERS+=	i915
. endif
.endif

.for _d in ${DRIVERS}
SYMLINKS+=	dril_dri.so ${DRIDIR}/${_d}_dri.so
SYMLINKS+=	dril_dri.so.${SHLIB_MAJOR} ${DRIDIR}/${_d}_dri.so.${SHLIB_MAJOR}
.if ${MKDEBUG} != "no"
SYMLINKS+=	dril_dri.so.${SHLIB_MAJOR}.debug \
		${DRIDEBUGDIR}/${_d}_dri.so.${SHLIB_MAJOR}.debug
.endif
.endfor

MKLINT=		no

.include <bsd.x11.mk>
.include <bsd.lib.mk>
