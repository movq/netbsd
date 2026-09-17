#	$NetBSD: Makefile,v 1.11 2021/04/07 09:57:02 simonb Exp $
#
#

.include <bsd.own.mk>

.if (defined(MKDTRACE) && ${MKDTRACE} != "no")
SUBDIR+=	libdtrace
SUBDIR+=	drti
.endif
.if ${MKCTF:Uno} != "no"
SUBDIR+=	libctf
.endif

.include <bsd.subdir.mk>
