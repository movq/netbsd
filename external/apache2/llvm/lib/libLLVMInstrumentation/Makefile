#	$NetBSD: Makefile,v 1.4 2023/06/03 08:52:53 lukem Exp $

LIB=	LLVMInstrumentation

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Instrumentation

SRCS+=	AddressSanitizer.cpp \
	AllocToken.cpp \
	BoundsChecking.cpp \
	CGProfile.cpp \
	ControlHeightReduction.cpp \
	DataFlowSanitizer.cpp \
	GCOVProfiling.cpp \
	BlockCoverageInference.cpp \
	MemProfInstrumentation.cpp \
	MemProfUse.cpp \
	MemorySanitizer.cpp \
	NumericalStabilitySanitizer.cpp \
	IndirectCallPromotion.cpp \
	InstrProfiling.cpp \
	KCFI.cpp \
	LowerAllowCheckPass.cpp \
	PGOCtxProfFlattening.cpp \
	PGOCtxProfLowering.cpp \
	PGOForceFunctionAttrs.cpp \
	PGOInstrumentation.cpp \
	PGOMemOPSizeOpt.cpp \
	SanitizerCoverage.cpp \
	SanitizerBinaryMetadata.cpp \
	ValueProfileCollector.cpp \
	ThreadSanitizer.cpp \
	TypeSanitizer.cpp \
	HWAddressSanitizer.cpp \
	RealtimeSanitizer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif

CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
