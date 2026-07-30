#	$NetBSD$

LIB=	LLVMTextAPIBinaryReader

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/TextAPI/BinaryReader

SRCS+=	DylibReader.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
