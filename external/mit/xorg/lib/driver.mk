#	$NetBSD: driver.mk,v 1.10 2026/07/10 05:06:56 kre Exp $

# stuff both dri and gallium drivers need.

# Utility, format, and xmlconfig sources are provided by private PIC archives.
.include "../mesa-private-libs.mk"

.PATH:		${X11SRCDIR.Mesa}/src/util/perf
.PATH:		${X11SRCDIR.Mesa}/../src/util/perf

CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/util

SRCS.perf= \
	u_trace.c

CPPFLAGS.u_trace.c+=	-I${X11SRCDIR.Mesa}/src/util/perf
CPPFLAGS.u_trace.c+=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary

SRCS+=	${SRCS.perf}
