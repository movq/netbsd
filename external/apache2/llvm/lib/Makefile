# $NetBSD: Makefile,v 1.4 2021/05/30 01:56:50 joerg Exp $

.include <bsd.own.mk>

SUBDIR=	\
	libLLVMAnalysis \
	libLLVMAsmParser \
	libLLVMAsmPrinter \
	libLLVMBinaryFormat \
	libLLVMBitReader \
	libLLVMBitWriter \
	libLLVMBitstreamReader \
	libLLVMCGData \
	libLLVMCodeGen \
	libLLVMCodeGenTypes \
	libLLVMDebugInfoCodeView \
	libLLVMDebugInfoDWARF \
	libLLVMDebugInfoDWARFLowLevel \
	libLLVMDebugInfoMSF \
	libLLVMDemangle \
	libLLVMExtensions \
	libLLVMFrontendAtomic \
	libLLVMFrontendDirective \
	libLLVMFrontendDriver \
	libLLVMFrontendHLSL \
	libLLVMFrontendOffloading \
	libLLVMFrontendOpenACC \
	libLLVMFrontendOpenMP \
	libLLVMGlobalISel \
	libLLVMHipStdPar \
	libLLVMIR \
	libLLVMIRPrinter \
	libLLVMIRReader \
	libLLVMInstCombine \
	libLLVMInstrumentation \
	libLLVMipo \
	libLLVMLTO \
	libLLVMLinker \
	libLLVMMC \
	libLLVMMCDisassembler \
	libLLVMMCParser \
	libLLVMObjCARC \
	libLLVMObject \
	libLLVMOption \
	libLLVMPasses \
	libLLVMPlugins \
	libLLVMProfileData \
	libLLVMProfileDataCoverage \
	libLLVMRemarks \
	libLLVMSandboxIR \
	libLLVMScalarOpts \
	libLLVMSelectionDAG \
	libLLVMSupport \
	libLLVMTableGenBasic \
	libLLVMTableGenCommon \
	libLLVMTableGen \
	libLLVMTarget \
	libLLVMTargetParser \
	libLLVMTextAPI \
	libLLVMTextAPIBinaryReader \
	libLLVMTransformsAggressiveInstCombine \
	libLLVMTransformsCFGuard \
	libLLVMTransformsCoroutines \
	libLLVMTransformsUtils \
	libLLVMVectorize \
	libLLVMWindowsDriver \
	libLLVMAArch64CodeGen \
	libLLVMARMCodeGen \
	libLLVMMipsCodeGen \
	libLLVMPowerPCCodeGen \
	libLLVMSparcCodeGen \
	libLLVMX86CodeGen

.if !defined(HOSTLIB) && !defined(_BSD_HOSTINIT_MK_)
SUBDIR+= \
	libLLVMAMDGPUCodeGen \
	libLLVMDebuginfod \
	libLLVMDebugInfoGSYM
.endif

SUBDIR+= .WAIT

.if !defined(HOSTLIB) && !defined(_BSD_HOSTINIT_MK_)
SUBDIR+= \
	libLLVMMIRParser \
	libLLVMAMDGPUMCTargetDesc \
	libLLVMAMDGPUTargetInfo \
	libLLVMAMDGPUAsmParser \
	libLLVMAMDGPUUtils
.endif

SUBDIR+= \
	libLLVMAArch64AsmParser \
	libLLVMAArch64Disassembler \
	libLLVMAArch64MCTargetDesc \
	libLLVMAArch64TargetInfo \
	libLLVMAArch64Utils

SUBDIR+= \
	libLLVMARMAsmParser \
	libLLVMARMDisassembler \
	libLLVMARMMCTargetDesc \
	libLLVMARMTargetInfo \
	libLLVMARMUtils

SUBDIR+= \
	libLLVMMipsAsmParser \
	libLLVMMipsDisassembler \
	libLLVMMipsMCTargetDesc \
	libLLVMMipsTargetInfo

SUBDIR+= \
	libLLVMPowerPCAsmParser \
	libLLVMPowerPCDisassembler \
	libLLVMPowerPCMCTargetDesc \
	libLLVMPowerPCTargetInfo

SUBDIR+= \
	libLLVMSparcAsmParser \
	libLLVMSparcDisassembler \
	libLLVMSparcMCTargetDesc \
	libLLVMSparcTargetInfo

SUBDIR+= \
	libLLVMX86AsmParser \
	libLLVMX86Disassembler \
	libLLVMX86MCTargetDesc \
	libLLVMX86TargetInfo

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes"
SUBDIR+= \
	libLLVMAMDGPUDisassembler \
	libLLVMExecutionEngine \
	libLLVMExecutionEngineJITLink \
	libLLVMExecutionEngineOrcShared \
	libLLVMExecutionEngineOrcTargetProcess \
	libLLVMFileCheck \
	libLLVMInterfaceStub \
	libLLVMMCJIT \
	libLLVMOrc \
	libLLVMRuntimeDyld
.endif

.if !defined(HOSTLIB) && !defined(_BSD_HOSTINIT_MK_)
SUBDIR+= \
	libLLVMDebugInfoPDB \
	libLLVMDebugInfoSymbolize
.endif

SUBDIR+= \
	libLLVMObjectYAML

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes"
SUBDIR+= \
	libLLVMHello \
	libLLVMInterpreter \
	libLLVMLineEditor \
	libLLVMToolDrivers \
	libLLVMToolDriversDlltool \
	libLLVMXRay
.endif

SUBDIR+= \
	libclangAPINotes \
	libclangAnalysis \
	libclangAnalysisLifetimeSafety \
	libclangAST \
	libclangASTMatchers \
	libclangBasic \
	libclangCodeGen \
	libclangCrossTU \
	libclangDriver \
	libclangEdit \
	libclangExtractAPI \
	libclangFormat \
	libclangFrontend \
	libclangFrontendRewrite \
	libclangFrontendTool \
	libclangIndex \
	libclangInstallAPI \
	libclangLex \
	libclangOptions \
	libclangParse \
	libclangRewrite \
	libclangSema \
	libclangSerialization \
	libclangStaticAnalyzerCheckers \
	libclangStaticAnalyzerCore \
	libclangSupport \
	libclangToolingInclusions \
	libclangToolingCore \
	.WAIT \
	libclangStaticAnalyzerFrontend

.if ${NO_LLVM_DEVELOPER:Uno} == "no" && ${LLVM_DEVELOPER:U} == "yes"
SUBDIR+= \
	libclangASTMatchersDynamic \
	libclangIndexSerialization \
	libclangInterpreter \
	libclangTesting \
	libclangTooling \
	libclangToolingDependencyScanning \
	libclangToolingRefactoring \
	libclangToolingSyntax
.endif

.include <bsd.subdir.mk>
