#	$NetBSD: Makefile,v 1.3 2014/03/25 09:52:55 ozaki-r Exp $

.include <bsd.own.mk>

.if ${MKCTF} != "no"
SUBDIR+=	ctfconvert ctfmerge ctfdump
.endif

# ztest is deferred until it is integrated with OpenZFS.

.include <bsd.subdir.mk>
