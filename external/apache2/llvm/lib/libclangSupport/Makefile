#	$NetBSD$

LIB=	clangSupport

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Support

SRCS+=	RISCVVIntrinsicUtils.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
