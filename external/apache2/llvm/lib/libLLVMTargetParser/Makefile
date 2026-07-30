#	$NetBSD$

LIB=	LLVMTargetParser

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/TargetParser

SRCS+=	AArch64TargetParser.cpp \
	ARMTargetParserCommon.cpp \
	ARMTargetParser.cpp \
	CSKYTargetParser.cpp \
	Host.cpp \
	LoongArchTargetParser.cpp \
	PPCTargetParser.cpp \
	RISCVISAInfo.cpp \
	RISCVTargetParser.cpp \
	SubtargetFeature.cpp \
	TargetParser.cpp \
	Triple.cpp \
	TargetDataLayout.cpp \
	X86TargetParser.cpp \
	XtensaTargetParser.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
