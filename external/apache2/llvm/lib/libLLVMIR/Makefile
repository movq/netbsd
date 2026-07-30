#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:52 joerg Exp $

LIB=	LLVMIR

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/IR

.include "${.PARSEDIR}/../../tablegen.mk"

SRCS+=	AbstractCallSite.cpp \
	AsmWriter.cpp \
	Assumptions.cpp \
	Attributes.cpp \
	AutoUpgrade.cpp \
	BasicBlock.cpp \
	BuiltinGCs.cpp \
	Comdat.cpp \
	ConstantFold.cpp \
	ConstantFPRange.cpp \
	ConstantRange.cpp \
	ConstantRangeList.cpp \
	Constants.cpp \
	ConvergenceVerifier.cpp \
	Core.cpp \
	CycleInfo.cpp \
	DIBuilder.cpp \
	DataLayout.cpp \
	DebugInfo.cpp \
	DebugInfoMetadata.cpp \
	DIExpressionOptimizer.cpp \
	DebugProgramInstruction.cpp \
	DebugLoc.cpp \
	DiagnosticHandler.cpp \
	DiagnosticInfo.cpp \
	DiagnosticPrinter.cpp \
	Dominators.cpp \
	DroppedVariableStats.cpp \
	DroppedVariableStatsIR.cpp \
	EHPersonalities.cpp \
	FPEnv.cpp \
	Function.cpp \
	GCStrategy.cpp \
	GVMaterializer.cpp \
	Globals.cpp \
	Intrinsics.cpp \
	NVVMIntrinsicUtils.cpp \
	IRBuilder.cpp \
	IRPrintingPasses.cpp \
	SSAContext.cpp \
	InlineAsm.cpp \
	Instruction.cpp \
	Instructions.cpp \
	IntrinsicInst.cpp \
	LLVMContext.cpp \
	LLVMContextImpl.cpp \
	LLVMRemarkStreamer.cpp \
	LegacyPassManager.cpp \
	MDBuilder.cpp \
	MemoryModelRelaxationAnnotations.cpp \
	Mangler.cpp \
	Metadata.cpp \
	Module.cpp \
	ModuleSummaryIndex.cpp \
	Operator.cpp \
	OptBisect.cpp \
	Pass.cpp \
	PassInstrumentation.cpp \
	PassManager.cpp \
	PassRegistry.cpp \
	PassTimingInfo.cpp \
	PrintPasses.cpp \
	ProfDataUtils.cpp \
	SafepointIRVerifier.cpp \
	ProfileSummary.cpp \
	PseudoProbe.cpp \
	ReplaceConstant.cpp \
	Statepoint.cpp \
	StructuralHash.cpp \
	Type.cpp \
	TypedPointerType.cpp \
	TypeFinder.cpp \
	Use.cpp \
	User.cpp \
	Value.cpp \
	ValueSymbolTable.cpp \
	VectorTypeUtils.cpp \
	Verifier.cpp \
	VFABIDemangler.cpp \
	RuntimeLibcalls.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
