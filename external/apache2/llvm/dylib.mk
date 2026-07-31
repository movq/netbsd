# $NetBSD$

# Component archives incorporated into the public LLVM and Clang shared
# libraries.  Keep the TableGen libraries out of libLLVM: they are build
# utilities and register command line options which must not become part of
# the public LLVM option namespace.

LLVM_SHLIB_FULLVERSION=	${LLVM_VERSION:R}
LLVM_SHLIB_MAJOR=	${LLVM_SHLIB_FULLVERSION:R}
LLVM_SHLIB_MINOR=	${LLVM_SHLIB_FULLVERSION:E}

LLVM_DYLIB_COMPONENTS=	\
	Analysis \
	AsmParser \
	AsmPrinter \
	BinaryFormat \
	BitReader \
	BitWriter \
	BitstreamReader \
	CGData \
	CodeGen \
	CodeGenTypes \
	DebugInfoBTF \
	DebugInfoCodeView \
	DebugInfoDWARF \
	DebugInfoDWARFLowLevel \
	DebugInfoGSYM \
	DebugInfoMSF \
	Debuginfod \
	Demangle \
	Extensions \
	FrontendAtomic \
	FrontendDirective \
	FrontendDriver \
	FrontendHLSL \
	FrontendOffloading \
	FrontendOpenACC \
	FrontendOpenMP \
	GlobalISel \
	HipStdPar \
	IR \
	IRPrinter \
	IRReader \
	InstCombine \
	Instrumentation \
	ipo \
	LTO \
	Linker \
	MC \
	MCDisassembler \
	MCParser \
	ObjCARC \
	Object \
	Option \
	Passes \
	Plugins \
	ProfileData \
	ProfileDataCoverage \
	Remarks \
	SandboxIR \
	ScalarOpts \
	SelectionDAG \
	Support \
	Target \
	TargetParser \
	TextAPI \
	TextAPIBinaryReader \
	TransformsAggressiveInstCombine \
	TransformsCFGuard \
	TransformsCoroutines \
	TransformsUtils \
	Vectorize \
	WindowsDriver \
	AArch64CodeGen \
	ARMCodeGen \
	MipsCodeGen \
	PowerPCCodeGen \
	SparcCodeGen \
	X86CodeGen \
	AMDGPUCodeGen \
	AMDGPUDisassembler \
	MIRParser \
	AMDGPUMCTargetDesc \
	AMDGPUTargetInfo \
	AMDGPUAsmParser \
	AMDGPUUtils \
	AArch64AsmParser \
	AArch64Disassembler \
	AArch64MCTargetDesc \
	AArch64TargetInfo \
	AArch64Utils \
	ARMAsmParser \
	ARMDisassembler \
	ARMMCTargetDesc \
	ARMTargetInfo \
	ARMUtils \
	MipsAsmParser \
	MipsDisassembler \
	MipsMCTargetDesc \
	MipsTargetInfo \
	PowerPCAsmParser \
	PowerPCDisassembler \
	PowerPCMCTargetDesc \
	PowerPCTargetInfo \
	SparcAsmParser \
	SparcDisassembler \
	SparcMCTargetDesc \
	SparcTargetInfo \
	X86AsmParser \
	X86Disassembler \
	X86MCTargetDesc \
	X86TargetInfo \
	DebugInfoPDB \
	DebugInfoSymbolize \
	ObjectYAML \
	ExecutionEngine \
	ExecutionEngineJITLink \
	ExecutionEngineOrcShared \
	ExecutionEngineOrcTargetProcess \
	MCJIT \
	Orc \
	RuntimeDyld

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes"
LLVM_DYLIB_COMPONENTS+=	\
	FileCheck \
	InterfaceStub \
	Hello \
	Interpreter \
	LineEditor \
	ToolDrivers \
	ToolDriversDlltool \
	XRay
.endif

CLANG_DYLIB_COMPONENTS=	\
	clangAPINotes \
	clangAnalysis \
	clangAnalysisLifetimeSafety \
	clangAST \
	clangASTMatchers \
	clangBasic \
	clangCodeGen \
	clangCrossTU \
	clangDriver \
	clangEdit \
	clangExtractAPI \
	clangFormat \
	clangFrontend \
	clangFrontendRewrite \
	clangFrontendTool \
	clangIndex \
	clangInstallAPI \
	clangLex \
	clangOptions \
	clangParse \
	clangRewrite \
	clangSema \
	clangSerialization \
	clangSupport \
	clangToolingInclusions \
	clangToolingCore

.if ${MKCLANGSTATICANALYZER} != "no"
CLANG_DYLIB_COMPONENTS+= \
	clangStaticAnalyzerCheckers \
	clangStaticAnalyzerCore \
	clangStaticAnalyzerFrontend
.endif

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes"
CLANG_DYLIB_COMPONENTS+=	\
	clangASTMatchersDynamic \
	clangIndexSerialization \
	clangInterpreter \
	clangTesting \
	clangTooling \
	clangToolingDependencyScanning \
	clangToolingRefactoring \
	clangToolingSyntax
.endif
