#	$NetBSD: Makefile,v 1.4 2017/09/17 00:00:39 sevan Exp $
.include <bsd.own.mk>

.if (${MKDTRACE} != "no")
SUBDIR+= dtracetoolkit
.endif

.if (${MKDTRACE} != "no" || ${MKCTF} != "no")
SUBDIR+= osnet
.endif

.if ${MKZFS} != "no"
SUBDIR+= openzfs
.endif

.include <bsd.subdir.mk>
