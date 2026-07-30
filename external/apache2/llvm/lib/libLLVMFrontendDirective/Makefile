#	$NetBSD$

LIB=	LLVMFrontendDirective
.include <bsd.init.mk>
.PATH: ${LLVM_SRCDIR}/lib/Frontend/Directive
SRCS+=	Spelling.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
