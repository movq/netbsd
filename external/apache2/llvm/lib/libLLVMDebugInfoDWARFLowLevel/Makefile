#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:51 joerg Exp $

LIB=	LLVMDebugInfoDWARFLowLevel

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/DWARF/LowLevel

SRCS+=	DWARFCFIProgram.cpp \
	DWARFExpression.cpp \
	DWARFUnwindTable.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
