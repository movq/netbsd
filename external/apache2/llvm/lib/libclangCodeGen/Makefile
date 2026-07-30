#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:56 joerg Exp $

LIB=	clangCodeGen

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/CodeGen

CPPFLAGS+=	-I${CLANG_SRCDIR}/lib/CodeGen \
		-I${CLANG_SRCDIR}/lib/CodeGen/TargetBuiltins \
		-I${CLANG_SRCDIR}/lib/CodeGen/Targets

SRCS+=	ABIInfo.cpp \
	ABIInfoImpl.cpp \
	BackendUtil.cpp \
	CGAtomic.cpp \
	CGBlocks.cpp \
	CGBuiltin.cpp \
	CGCUDANV.cpp \
	CGCUDARuntime.cpp \
	CGCXX.cpp \
	CGCXXABI.cpp \
	CGCall.cpp \
	CGClass.cpp \
	CGCleanup.cpp \
	CGCoroutine.cpp \
	CGDebugInfo.cpp \
	CGDecl.cpp \
	CGDeclCXX.cpp \
	CGException.cpp \
	CGExpr.cpp \
	CGExprAgg.cpp \
	CGExprCXX.cpp \
	CGExprComplex.cpp \
	CGExprConstant.cpp \
	CGExprScalar.cpp \
	CGGPUBuiltin.cpp \
	CGHLSLRuntime.cpp \
	CGHLSLBuiltins.cpp \
	CGLoopInfo.cpp \
	CGNonTrivialStruct.cpp \
	CGObjC.cpp \
	CGObjCGNU.cpp \
	CGObjCMac.cpp \
	CGObjCRuntime.cpp \
	CGOpenCLRuntime.cpp \
	CGOpenMPRuntime.cpp \
	CGOpenMPRuntimeGPU.cpp \
	CGPointerAuth.cpp \
	CGRecordLayoutBuilder.cpp \
	CGStmt.cpp \
	CGStmtOpenMP.cpp \
	CGVTT.cpp \
	CGVTables.cpp \
	CodeGenABITypes.cpp \
	CodeGenAction.cpp \
	CodeGenFunction.cpp \
	CodeGenModule.cpp \
	CodeGenPGO.cpp \
	CodeGenSYCL.cpp \
	CodeGenTBAA.cpp \
	CodeGenTypes.cpp \
	ConstantInitBuilder.cpp \
	CoverageMappingGen.cpp \
	ItaniumCXXABI.cpp \
	HLSLBufferLayoutBuilder.cpp \
	LinkInModulesPass.cpp \
	MacroPPCallbacks.cpp \
	MicrosoftCXXABI.cpp \
	ModuleBuilder.cpp \
	ObjectFilePCHContainerWriter.cpp \
	PatternInit.cpp \
	SanitizerMetadata.cpp \
	SwiftCallingConv.cpp \
	TrapReasonBuilder.cpp \
	VarBypassDetector.cpp \
	TargetInfo.cpp

TARGET_BUILTINS_SRCS=	ARM.cpp \
	AMDGPU.cpp \
	DirectX.cpp \
	Hexagon.cpp \
	NVPTX.cpp \
	PPC.cpp \
	RISCV.cpp \
	SPIR.cpp \
	SystemZ.cpp \
	WebAssembly.cpp \
	X86.cpp

.for _src in ${TARGET_BUILTINS_SRCS}
TargetBuiltins${_src}: ${CLANG_SRCDIR}/lib/CodeGen/TargetBuiltins/${_src}
	${TOOL_CAT} ${.ALLSRC} > ${.TARGET}
.endfor
SRCS+=	${TARGET_BUILTINS_SRCS:S,^,TargetBuiltins,}
CLEANFILES+=	${TARGET_BUILTINS_SRCS:S,^,TargetBuiltins,}

TARGETS_SRCS=	AArch64.cpp \
	AMDGPU.cpp \
	ARC.cpp \
	ARM.cpp \
	AVR.cpp \
	BPF.cpp \
	CSKY.cpp \
	DirectX.cpp \
	Hexagon.cpp \
	Lanai.cpp \
	LoongArch.cpp \
	M68k.cpp \
	MSP430.cpp \
	Mips.cpp \
	NVPTX.cpp \
	PPC.cpp \
	RISCV.cpp \
	SPIR.cpp \
	Sparc.cpp \
	SystemZ.cpp \
	TCE.cpp \
	VE.cpp \
	WebAssembly.cpp \
	X86.cpp \
	XCore.cpp

.for _src in ${TARGETS_SRCS}
Targets${_src}: ${CLANG_SRCDIR}/lib/CodeGen/Targets/${_src}
	${TOOL_CAT} ${.ALLSRC} > ${.TARGET}
.endfor
SRCS+=	${TARGETS_SRCS:S,^,Targets,}
CLEANFILES+=	${TARGETS_SRCS:S,^,Targets,}

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
