#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:46 joerg Exp $

PROG_CXX=	clang
NOMAN=		yes

.if !defined(HOSTPROG)
SYMLINKS+=	clang ${BINDIR}/clang++
SYMLINKS+=	clang ${BINDIR}/clang-cpp
.endif

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/tools/driver

SRCS=	clang-driver.cpp \
	driver.cpp \
	cc1_main.cpp \
	cc1as_main.cpp \
	cc1gen_reproducer_main.cpp

CLANG_LIBS+= \
	clangFrontendTool \
	clangExtractAPI \
	clangInstallAPI \
	clangFrontend \
	clangCrossTU \
	clangIndex \
	clangCodeGen \
	clangFrontendRewrite \
	clangFrontend \
	clangSerialization \
	clangParse \
	clangSema \
	clangAPINotes \
	clangAnalysisLifetimeSafety \
	clangAnalysis \
	clangASTMatchers \
	clangFormat \
	clangToolingInclusions \
	clangToolingCore \
	clangEdit \
	clangDriver \
	clangOptions \
	clangRewrite \
	clangAST \
	clangLex \
	clangSupport \
	clangBasic

.if !defined(HOSTPROG) && ${MKCLANGSTATICANALYZER} != "no"
CLANG_LIBS+= \
	clangStaticAnalyzerFrontend \
	clangStaticAnalyzerCheckers \
	clangStaticAnalyzerCore
.endif

LLVM_LIBS+= \
	FrontendDriver \
	WindowsDriver \
	TextAPIBinaryReader \
	FrontendHLSL \
	FrontendOpenACC \
	FrontendOpenMP \
	FrontendOffloading \
	FrontendAtomic \
	FrontendDirective \
	ProfileDataCoverage \
	LTO \
	Passes \
	TransformsAggressiveInstCombine \
	TransformsCoroutines \
	HipStdPar \
	ObjCARC \
	Extensions \
	Plugins \
	IRReader \
	AsmParser \
	BitstreamReader \
	BitReader \
	BitWriter

.if !defined(HOSTPROG)
LLVM_LIBS+= \
	AMDGPUCodeGen \
	AMDGPUTargetInfo \
	AMDGPUAsmParser \
	AMDGPUMCTargetDesc \
	AMDGPUUtils \
	MIRParser
.endif

LLVM_LIBS+= \
	AArch64CodeGen \
	AArch64TargetInfo \
	AArch64AsmParser \
	AArch64MCTargetDesc \
	AArch64Utils \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMUtils \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmParser \
	MipsMCTargetDesc \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmParser \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	SparcAsmParser \
	X86CodeGen \
	X86MCTargetDesc \
	X86TargetInfo \
	X86AsmParser \
	ipo \
	GlobalISel \
	SelectionDAG \
	AsmPrinter \
	CodeGen \
	CGData \
	CodeGenTypes \
	Vectorize \
	SandboxIR \
	Target \
	InstCombine \
	Instrumentation \
	ScalarOpts \
	ObjectYAML \
	ProfileData \
	DebugInfoDWARF \
	MCDisassembler \
	MCParser \
	MC \
	DebugInfoDWARFLowLevel \
	DebugInfoCodeView \
	DebugInfoMSF \
	TransformsUtils \
	IRPrinter \
	Analysis \
	Object \
	TextAPI \
	Linker \
	TransformsCFGuard \
	IR \
	Remarks \
	Option \
	BinaryFormat \
	TargetParser \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
