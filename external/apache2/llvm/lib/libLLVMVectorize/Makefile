#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:55 joerg Exp $

LIB=	LLVMVectorize

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Vectorize

SRCS+=	LoadStoreVectorizer.cpp \
	LoopIdiomVectorize.cpp \
	LoopVectorizationLegality.cpp \
	LoopVectorize.cpp \
	SLPVectorizer.cpp \
	Vectorize.cpp \
	VectorCombine.cpp \
	VPlan.cpp \
	VPlanAnalysis.cpp \
	VPlanConstruction.cpp \
	VPlanPredicator.cpp \
	VPlanRecipes.cpp \
	VPlanSLP.cpp \
	VPlanTransforms.cpp \
	VPlanUnroll.cpp \
	VPlanVerifier.cpp \
	VPlanUtils.cpp

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Vectorize/SandboxVectorizer

SRCS+=	DependencyGraph.cpp \
	InstrMaps.cpp \
	Interval.cpp \
	Legality.cpp \
	SandboxVectorizer.cpp \
	SandboxVectorizerPassBuilder.cpp \
	Scheduler.cpp \
	SeedCollector.cpp \
	VecUtils.cpp

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Vectorize/SandboxVectorizer/Passes

SRCS+=	BottomUpVec.cpp \
	PackReuse.cpp \
	RegionsFromBBs.cpp \
	RegionsFromMetadata.cpp \
	SeedCollection.cpp \
	TransactionAcceptOrRevert.cpp \
	TransactionSave.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
