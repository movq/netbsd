#	$NetBSD$

LIB=	LLVMExtensions
.include <bsd.init.mk>
.PATH: ${LLVM_SRCDIR}/lib/Extensions
SRCS+=	Extensions.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
