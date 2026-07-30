#	$NetBSD$

LIB=	LLVMPlugins
.include <bsd.init.mk>
.PATH: ${LLVM_SRCDIR}/lib/Plugins
SRCS+=	PassPlugin.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
