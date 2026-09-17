#	$NetBSD: Makefile,v 1.2 2010/02/21 01:46:36 darran Exp $

.include <bsd.own.mk>

.if ${MKDTRACE} != "no"
SUBDIR+=	dtrace
.endif

# zdb is deferred until it is integrated with OpenZFS.

.include <bsd.subdir.mk>
