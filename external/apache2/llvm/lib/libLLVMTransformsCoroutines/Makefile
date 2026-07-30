#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:13 joerg Exp $

LIB=	LLVMTransformsCoroutines

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Coroutines

SRCS+=	Coroutines.cpp \
	CoroAnnotationElide.cpp \
	CoroCleanup.cpp \
	CoroConditionalWrapper.cpp \
	CoroEarly.cpp \
	CoroElide.cpp \
	CoroFrame.cpp \
	CoroSplit.cpp \
	SuspendCrossingInfo.cpp \
	SpillUtils.cpp \
	MaterializationUtils.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
