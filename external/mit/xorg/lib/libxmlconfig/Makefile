#	$NetBSD$

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIB=		xmlconfig
LIBISPRIVATE=	pic
NOSTATICLIB=	yes
MKLINT=		no

MESA_SRC_MODULES=
.include "../libmesa.mk"

.PATH:		${X11SRCDIR.Mesa}/src/util
SRCS=		xmlconfig.c
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/util

.include <bsd.x11.mk>
.include <bsd.lib.mk>
