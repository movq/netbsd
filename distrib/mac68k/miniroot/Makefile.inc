#	$NetBSD: Makefile.inc,v 1.4 2002/03/06 12:04:51 lukem Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${ARCHDIR}/termcap
