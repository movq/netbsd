#	$NetBSD: Makefile,v 1.36 2026/07/05 08:42:40 mrg Exp $

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIB=		GL

SUBDIR=		internal

# glx
.PATH:		${X11SRCDIR.Mesa}/src/glx
SRCS.glx=	\
	clientattrib.c \
	clientinfo.c \
	compsize.c \
	create_context.c \
	eval.c \
	glx_error.c \
	glx_pbuffer.c \
	glx_query.c \
	glxcmds.c \
	glxconfig.c \
	MESAindirect.c \
	MESAindirect_init.c \
	MESAindirect_size.c \
	MESAindirect_window_pos.c \
	glxcurrent.c \
	glxext.c \
	glxextensions.c \
	glxhash.c \
	indirect_glx.c \
	indirect_texture_compression.c \
	indirect_transpose_matrix.c \
	indirect_vertex_array.c \
	indirect_vertex_program.c \
	pixel.c \
	pixelstore.c \
	query_renderer.c \
	render2.c \
	renderpix.c \
	single2.c \
	vertarr.c \

BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/../src/mesa/glapi/glapi/gen/indirect_init.c MESAindirect_init.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/glx/indirect_window_pos.c MESAindirect_window_pos.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/../src/mesa/glapi/glapi/gen/indirect.c MESAindirect.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/../src/mesa/glapi/glapi/gen/indirect_size.c MESAindirect_size.c

# glapi
.PATH:		${X11SRCDIR.Mesa}/src/mesa/glapi/glapi
SRCS.glapi=	libgl_public.c

.for _f in ${SRCS.glx}
CPPFLAGS.${_f}= -I${X11SRCDIR.Mesa}/../src/mesa/glapi/glapi/gen \
		-I${X11SRCDIR.Mesa}/src \
		-I${X11SRCDIR.Mesa}/src/mesa \
		-I${X11SRCDIR.Mesa}/../src/mesa \
		-DGLX_SHARED_GLAPI
.endfor

.for _f in ${SRCS.glapi}
CPPFLAGS.${_f}= -I${X11SRCDIR.Mesa}/src \
		-I${X11SRCDIR.Mesa}/src/mesa \
		-I${X11SRCDIR.Mesa}/../src/mesa/glapi/glapi/gen
.endfor


.if ${X11DRI} != "no"
# dri
SRCS.dri=	\
	dri_common.c \
	dri_common_query_renderer.c \
	xfont.c \
	drisw_glx.c \
	dri3_glx.c

.for fn in ${SRCS.dri}
CPPFLAGS.${fn}= \
	-I${X11SRCDIR.Mesa}/src \
	-DGL_LIB_NAME="\"libGL.so.${SHLIB_MAJOR}\""
.endfor
.endif

.include "../libloader.mk"
.include "../mesa-private-libs.mk"

.PATH:		${X11SRCDIR.Mesa}/src/x11
SRCS.x11=	x11_display.c x11_dri3.c

.PATH:		${X11SRCDIR.Mesa}/src/glw
.PATH:		${X11SRCDIR.Mesa}/include/GL
.PATH:		${X11SRCDIR.Mesa}/src/mesa/drivers/x11
INCS=	gl.h glcorearb.h glext.h \
	glx.h glxext.h \
	wglext.h
INCSDIR=${X11INCDIR}/GL


SRCS+=	${SRCS.dri} ${SRCS.x11} ${SRCS.glx} ${SRCS.glapi}

CPPFLAGS+= \
	-I${X11SRCDIR.Mesa}/include \
	-I${X11SRCDIR.Mesa}/include/GL/internal \
	-I${X11SRCDIR.Mesa}/src/loader \
	-I${X11SRCDIR.Mesa}/src/x11 \
	-I${X11SRCDIR.Mesa}/src \
	-I${X11SRCDIR.Mesa}/../src \
	-I${X11SRCDIR.Mesa}/src/mesa \
	-I${X11SRCDIR.Mesa}/src/mesa/glapi \
	-I${X11SRCDIR.Mesa}/src/mesa/glapi/glapi \
	-I${X11SRCDIR.Mesa}/../src/mesa/glapi/glapi/gen \
	-I${X11SRCDIR.Mesa}/src/glx \
	-I${X11SRCDIR.Mesa}/src/gallium/include \
	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary \
	-I${X11SRCDIR.Mesa}/src/gallium/frontends/dri \
	-I${DESTDIR}${X11INCDIR}/libdrm \
	-I${DESTDIR}${X11INCDIR}/GL \
	-I${DESTDIR}${X11INCDIR}/X11 \
	-I${DESTDIR}${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}

#		-D_NETBSD_SOURCE \

# old
CPPFLAGS+=	-D__GLX_ALIGN64 \
		-DXF86VIDMODE \
		-DGLX_SHARED_GLAPI

# new
CPPFLAGS+=	\
	-DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 \
	-DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 \
	-DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 \
	-DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DYYTEXT_POINTER=1 \
	-DHAVE___BUILTIN_BSWAP32=1 -DHAVE___BUILTIN_BSWAP64=1 \
	-DHAVE___BUILTIN_CLZ=1 -DHAVE___BUILTIN_CLZLL=1 \
	-DHAVE___BUILTIN_CTZ=1 -DHAVE___BUILTIN_EXPECT=1 \
	-DHAVE___BUILTIN_FFS=1 -DHAVE___BUILTIN_FFSLL=1 \
	-DHAVE___BUILTIN_POPCOUNT=1 -DHAVE___BUILTIN_POPCOUNTLL=1 \
	-DHAVE___BUILTIN_UNREACHABLE=1 -DHAVE_FUNC_ATTRIBUTE_CONST=1 \
	-DHAVE_FUNC_ATTRIBUTE_FLATTEN=1 -DHAVE_FUNC_ATTRIBUTE_FORMAT=1 \
	-DHAVE_FUNC_ATTRIBUTE_MALLOC=1 -DHAVE_FUNC_ATTRIBUTE_PACKED=1 \
	-DHAVE_FUNC_ATTRIBUTE_PURE=1 -DHAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL=1 \
	-DHAVE_FUNC_ATTRIBUTE_UNUSED=1 -DHAVE_FUNC_ATTRIBUTE_VISIBILITY=1 \
	-DHAVE_FUNC_ATTRIBUTE_WARN_UNUSED_RESULT=1 \
	-DHAVE_FUNC_ATTRIBUTE_WEAK=1 -DHAVE_FUNC_ATTRIBUTE_ALIAS=1 \
	-DHAVE_FUNC_ATTRIBUTE_NORETURN=1 -DHAVE_ENDIAN_H=1 -DHAVE_DLADDR=1 \
	-DHAVE_CLOCK_GETTIME=1 -DHAVE_PTHREAD_PRIO_INHERIT=1 \
	-DHAVE_PTHREAD=1 -DENABLE_ST_OMX_BELLAGIO=0 -DENABLE_ST_OMX_TIZONIA=0 \
	-DHAVE_TIMESPEC_GET -DHAVE_STRUCT_TIMESPEC -DHAVE_SYS_SHM_H \
	-DGLX_USE_TLS \
	-DHAVE_NOATEXIT

.include "../asm.mk"

CPPFLAGS+=	\
	-DHAVE_LIBDRM -DGLX_USE_DRM \
	-DGLX_INDIRECT_RENDERING -DGLX_DIRECT_RENDERING \
	-DHAVE_ALIAS -DMESA_EGL_NO_X11_HEADERS \
	-Ddri_destroy_context=glx_dri_destroy_context \
	-Ddri_unbind_context=glx_dri_unbind_context

CPPFLAGS+=	\
	-DUSE_EXTERNAL_DXTN_LIB=1 \
	-DYYTEXT_POINTER=1

CFLAGS+=	-fvisibility=hidden -fno-strict-aliasing -fno-builtin-memcmp
CFLAGS+=	-pthread

LIBDPLIBS=	gallium	${.CURDIR}/../gallium \
		Xext		${.CURDIR}/../libXext \
		X11		${.CURDIR}/../libX11/dynamic \
		Xxf86vm		${.CURDIR}/../libXxf86vm \
		Xfixes		${.CURDIR}/../libXfixes \
		Xdamage		${.CURDIR}/../libXdamage \
		drm		${.CURDIR}/../libdrm \
		X11-xcb		${.CURDIR}/../libX11/libX11-xcb \
		xcb-dri2	${.CURDIR}/../libxcb/dri2 \
		xcb-dri3	${.CURDIR}/../libxcb/dri3 \
		xcb-glx		${.CURDIR}/../libxcb/glx \
		xcb-present	${.CURDIR}/../libxcb/present \
		xcb-randr	${.CURDIR}/../libxcb/randr \
		xcb-shm		${.CURDIR}/../libxcb/shm \
		xcb-sync	${.CURDIR}/../libxcb/sync \
		xcb-xfixes	${.CURDIR}/../libxcb/xfixes \
		xshmfence	${.CURDIR}/../libxshmfence \
		expat		${NETBSDSRCDIR}/external/mit/expat/lib/libexpat \
		terminfo	${NETBSDSRCDIR}/lib/libterminfo \
		execinfo	${NETBSDSRCDIR}/lib/libexecinfo \
		elf		${NETBSDSRCDIR}/external/bsd/elftoolchain/lib/libelf \
		z		${NETBSDSRCDIR}/lib/libz \
		m		${NETBSDSRCDIR}/lib/libm \
		pthread		${NETBSDSRCDIR}/lib/libpthread

MKLINT=no

PKGCONFIG=	gl
PKGDIST.gl=	${X11SRCDIR.Mesa}/../src/pkgconfig
.include "${.CURDIR}/../libGL${OLD_SUFFIX}/mesa-ver.mk"
CPPFLAGS+=	-DPACKAGE_VERSION=\"${MESA_VER}\"
PKGCONFIG_VERSION.gl=	${MESA_VER}

# XXX remove these from bsd.x11.mk

PKGCONFIG_SED_FLAGS= \
	-e "s,@GL_PC_REQ_PRIV@,libdrm x11 xext xdamage xfixes xcb-glx xcb-dri2 xxf86vm,; \
	    s,@GL_PKGCONF_LIB@,GL,; \
	    s,@GL_PC_LIB_PRIV@,-lm -lpthread -pthread,; \
	    s,@GL_PC_CFLAGS@,,; \
	    s,@GLX_TLS@,yes,"


CWARNFLAGS.clang+=	-Wno-tautological-compare -Wno-format -Wno-constant-conversion \
			-Wno-error=incompatible-pointer-types -Wno-error=atomic-alignment
CWARNFLAGS.gcc+=	${CC_WNO_STRINGOP_OVERFLOW}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
