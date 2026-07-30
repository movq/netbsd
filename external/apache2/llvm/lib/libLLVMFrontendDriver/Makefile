#	$NetBSD$

LIB=	LLVMFrontendDriver
.include <bsd.init.mk>
.PATH: ${LLVM_SRCDIR}/lib/Frontend/Driver
SRCS+=	CodeGenOptions.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
