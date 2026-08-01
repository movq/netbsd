#	$NetBSD$

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIB=		mesa_util
LIBISPRIVATE=	pic
NOSTATICLIB=	yes
LIBISCXX=	yes
MKLINT=		no

MESA_SRC_MODULES=
.include "../libmesa.mk"
.include "../mesa-util.mk"

SRCS=		${SRCS.mesa_util}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm \
		-I${X11SRCDIR.drm}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
