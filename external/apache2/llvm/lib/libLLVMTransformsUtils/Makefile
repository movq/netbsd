#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:55 joerg Exp $

LIB=	LLVMTransformsUtils

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Utils

SRCS+=	AddDiscriminators.cpp \
	AMDGPUEmitPrintf.cpp \
	ASanStackFrameLayout.cpp \
	AssumeBundleBuilder.cpp \
	BasicBlockUtils.cpp \
	BreakCriticalEdges.cpp \
	BuildLibCalls.cpp \
	BypassSlowDivision.cpp \
	CallPromotionUtils.cpp \
	CallGraphUpdater.cpp \
	CanonicalizeAliases.cpp \
	CanonicalizeFreezeInLoops.cpp \
	CloneFunction.cpp \
	CloneModule.cpp \
	CodeExtractor.cpp \
	CodeLayout.cpp \
	CodeMoverUtils.cpp \
	ControlFlowUtils.cpp \
	CtorUtils.cpp \
	CountVisits.cpp \
	Debugify.cpp \
	DebugSSAUpdater.cpp \
	DeclareRuntimeLibcalls.cpp \
	DemoteRegToStack.cpp \
	DXILUpgrade.cpp \
	EntryExitInstrumenter.cpp \
	EscapeEnumerator.cpp \
	Evaluator.cpp \
	FixIrreducible.cpp \
	FlattenCFG.cpp \
	FunctionComparator.cpp \
	FunctionImportUtils.cpp \
	GlobalStatus.cpp \
	GuardUtils.cpp \
	HelloWorld.cpp \
	InlineFunction.cpp \
	InjectTLIMappings.cpp \
	InstructionNamer.cpp \
	Instrumentation.cpp \
	IntegerDivision.cpp \
	IRNormalizer.cpp \
	LCSSA.cpp \
	LibCallsShrinkWrap.cpp \
	Local.cpp \
	LoopConstrainer.cpp \
	LoopPeel.cpp \
	LoopRotationUtils.cpp \
	LoopSimplify.cpp \
	LoopUnroll.cpp \
	LoopUnrollAndJam.cpp \
	LoopUnrollRuntime.cpp \
	LoopUtils.cpp \
	LoopVersioning.cpp \
	LowerAtomic.cpp \
	LowerGlobalDtors.cpp \
	LowerIFunc.cpp \
	LowerInvoke.cpp \
	LowerMemIntrinsics.cpp \
	LowerSwitch.cpp \
	LowerVectorIntrinsics.cpp \
	MatrixUtils.cpp \
	MemoryOpRemark.cpp \
	MemoryTaggingSupport.cpp \
	Mem2Reg.cpp \
	MetaRenamer.cpp \
	MisExpect.cpp \
	ModuleUtils.cpp \
	MoveAutoInit.cpp \
	NameAnonGlobals.cpp \
	PredicateInfo.cpp \
	ProfileVerify.cpp \
	PromoteMemoryToRegister.cpp \
	RelLookupTableConverter.cpp \
	ScalarEvolutionExpander.cpp \
	SCCPSolver.cpp \
	StripGCRelocates.cpp \
	SSAUpdater.cpp \
	SSAUpdaterBulk.cpp \
	SampleProfileInference.cpp \
	SampleProfileLoaderBaseUtil.cpp \
	SanitizerStats.cpp \
	SimplifyCFG.cpp \
	SimplifyIndVar.cpp \
	SimplifyLibCalls.cpp \
	SizeOpts.cpp \
	SplitModule.cpp \
	SplitModuleByCategory.cpp \
	StripNonLineTableDebugInfo.cpp \
	SymbolRewriter.cpp \
	UnifyFunctionExitNodes.cpp \
	UnifyLoopExits.cpp \
	Utils.cpp \
	ValueMapper.cpp \
	VNCoercion.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
