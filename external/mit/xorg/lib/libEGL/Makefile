#	$NetBSD: Makefile,v 1.10 2026/07/05 08:42:39 mrg Exp $

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIB=		EGL

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

#SUBDIR=		internal

# egl
.PATH:		${X11SRCDIR.Mesa}/src/egl/main
SRCS.egl= \
	eglapi.c \
	eglarray.c \
	eglconfig.c \
	eglconfigdebug.c \
	eglcontext.c \
	eglcurrent.c \
	egldevice.c \
	egldisplay.c \
	eglglobals.c \
	eglimage.c \
	egllog.c \
	eglsurface.c \
	eglsync.c

.PATH:		${X11SRCDIR.Mesa}/src/egl/drivers/dri2
SRCS.drivers/dri2= \
	egl_dri2.c \
	platform_x11.c \
	platform_x11_dri3.c \
	platform_device.c \
	platform_drm.c \
	platform_surfaceless.c

.PATH:		${X11SRCDIR.Mesa}/src/x11
SRCS.x11= \
	x11_display.c \
	x11_dri3.c

.PATH:		${X11SRCDIR.Mesa}/include/EGL
INCS=	\
	egl.h \
	eglext.h \
	eglmesaext.h \
	eglplatform.h
INCSDIR=${X11INCDIR}/EGL

SRCS+=	${SRCS.egl} ${SRCS.drivers/dri2} ${SRCS.x11}

CPPFLAGS+= \
	-I${X11SRCDIR.Mesa}/include \
	-I${X11SRCDIR.Mesa}/include/drm-uapi \
	-I${X11SRCDIR.Mesa}/src/gbm/main \
	-I${X11SRCDIR.Mesa}/src/egl/main \
	-I${X11SRCDIR.Mesa}/src/gbm/backends/dri \
	-I${X11SRCDIR.Mesa}/src/loader \
	-I${X11SRCDIR.Mesa}/src/x11 \
	-I${X11SRCDIR.Mesa}/src \
	-I${X11SRCDIR.Mesa}/../src \
	-I${X11SRCDIR.Mesa}/src/mesa \
	-I${X11SRCDIR.Mesa}/../src/mesa/glapi/glapi/gen \
	-I${X11SRCDIR.Mesa}/src/gallium/include \
	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary \
	-I${X11SRCDIR.Mesa}/src/gallium/frontends/dri \
	-I${X11SRCDIR.Mesa}/../src/mesa/drivers/dri/common \
	-I${X11SRCDIR.drm} \
	-I${DESTDIR}${X11INCDIR}/libdrm

EGL_DRIVER_INSTALL_DIR=${X11ROOTDIR}/lib
CPPFLAGS+= \
	-DDEFAULT_DRIVER_DIR=\"${X11USRLIBDIR}/modules/dri\" \
	-D_EGL_NATIVE_PLATFORM=_EGL_PLATFORM_X11 \
	-D_EGL_DRIVER_SEARCH_DIR=\"$(EGL_DRIVER_INSTALL_DIR)\" \
	-D_EGL_OS_UNIX=1

CPPFLAGS+=	${X11FLAGS.THREADLIB} \
		-DHAVE_X11_PLATFORM \
		-DHAVE_DRM_PLATFORM \
		-DHAVE_TIMESPEC_GET \
		-DHAVE_STRUCT_TIMESPEC \
		-DHAVE_SYS_SHM_H \
		-DHAVE_PTHREAD \
		-DHAVE_LIBDRM \
		-DHAVE_MINCORE \
		-DHAVE_NOATEXIT

.include "../libloader.mk"
.include "../mesa-private-libs.mk"

CFLAGS+=	-fvisibility=hidden -fno-strict-aliasing -fno-builtin-memcmp
CFLAGS+=	-pthread
LDFLAGS+=	-Wl,--version-script=${X11SRCDIR.Mesa}/src/egl/egl.sym

LIBDPLIBS=	gallium	${.CURDIR}/../gallium \
		Xext		${.CURDIR}/../libXext \
		X11		${.CURDIR}/../libX11/dynamic \
		Xfixes		${.CURDIR}/../libXfixes \
		Xdamage		${.CURDIR}/../libXdamage \
		Xdmcp		${.CURDIR}/../libXdmcp \
		drm		${.CURDIR}/../libdrm \
		gbm		${.CURDIR}/../libgbm${OLD_SUFFIX} \
		xshmfence	${.CURDIR}/../libxshmfence \
		X11-xcb		${.CURDIR}/../libX11/libX11-xcb \
		xcb-dri2	${.CURDIR}/../libxcb/dri2 \
		xcb-dri3	${.CURDIR}/../libxcb/dri3 \
		xcb-present	${.CURDIR}/../libxcb/present \
		xcb-randr	${.CURDIR}/../libxcb/randr \
		xcb-glx		${.CURDIR}/../libxcb/glx \
		xcb-shm		${.CURDIR}/../libxcb/shm \
		xcb-xfixes	${.CURDIR}/../libxcb/xfixes \
		xcb-sync	${.CURDIR}/../libxcb/sync \
		xcb		${.CURDIR}/../libxcb/libxcb \
		expat		${NETBSDSRCDIR}/external/mit/expat/lib/libexpat \
		terminfo	${NETBSDSRCDIR}/lib/libterminfo \
		execinfo	${NETBSDSRCDIR}/lib/libexecinfo \
		elf		${NETBSDSRCDIR}/external/bsd/elftoolchain/lib/libelf \
		z		${NETBSDSRCDIR}/lib/libz \
		m		${NETBSDSRCDIR}/lib/libm \
		pthread		${NETBSDSRCDIR}/lib/libpthread

MKLINT=no

PKGCONFIG=	egl
PKGDIST.egl=	${X11SRCDIR.Mesa}/../src/pkgconfig
.include "${.CURDIR}/../libGL/mesa-ver.mk"
CPPFLAGS+=	-DPACKAGE_VERSION=\"${MESA_VER}\"
PKGCONFIG_VERSION.egl=	${MESA_VER}

CWARNFLAGS.clang+=	-Wno-tautological-compare -Wno-format -Wno-constant-conversion
COPTS.platform_drm.c+=	-Wno-error=stack-protector

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
