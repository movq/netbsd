# $NetBSD: Makefile,v 1.1.2.3 2001/07/29 20:32:33 he Exp $
#
# Build umount(8) without network support
#

SRCDIR= ${.CURDIR}/../../../sbin/umount
PROG=	umount
MKMAN=	no
LDSTATIC?=-static
CPPFLAGS+= -DSMALL
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
WARNS?=	1

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
