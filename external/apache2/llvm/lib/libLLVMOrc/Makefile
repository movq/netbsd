#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:53 joerg Exp $

LIB=	LLVMOrc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ExecutionEngine/Orc

SRCS+=	AbsoluteSymbols.cpp \
	BacktraceTools.cpp \
	COFF.cpp \
	COFFVCRuntimeSupport.cpp \
	COFFPlatform.cpp \
	CompileOnDemandLayer.cpp \
	CompileUtils.cpp \
	Core.cpp \
	DebugUtils.cpp \
	EHFrameRegistrationPlugin.cpp \
	EPCDynamicLibrarySearchGenerator.cpp \
	EPCGenericDylibManager.cpp \
	EPCGenericJITLinkMemoryManager.cpp \
	EPCGenericRTDyldMemoryManager.cpp \
	EPCIndirectionUtils.cpp \
	ExecutionUtils.cpp \
	ExecutorResolutionGenerator.cpp \
	ObjectFileInterface.cpp \
	IndirectionUtils.cpp \
	InProcessMemoryAccess.cpp \
	IRCompileLayer.cpp \
	IRTransformLayer.cpp \
	IRPartitionLayer.cpp \
	JITTargetMachineBuilder.cpp \
	JITLinkReentryTrampolines.cpp \
	LazyObjectLinkingLayer.cpp \
	LazyReexports.cpp \
	Layer.cpp \
	LinkGraphLayer.cpp \
	LinkGraphLinkingLayer.cpp \
	LoadLinkableFile.cpp \
	LookupAndRecordAddrs.cpp \
	LLJIT.cpp \
	MachO.cpp \
	MachOPlatform.cpp \
	MapperJITLinkMemoryManager.cpp \
	MemoryMapper.cpp \
	ELFNixPlatform.cpp \
	Mangling.cpp \
	ObjectLinkingLayer.cpp \
	ObjectTransformLayer.cpp \
	OrcABISupport.cpp \
	OrcV2CBindings.cpp \
	RTDyldObjectLinkingLayer.cpp \
	SectCreate.cpp \
	SelfExecutorProcessControl.cpp \
	SimpleRemoteEPC.cpp \
	SimpleRemoteMemoryMapper.cpp \
	Speculation.cpp \
	SpeculateAnalyses.cpp \
	ExecutorProcessControl.cpp \
	TaskDispatch.cpp \
	ThreadSafeModule.cpp \
	UnwindInfoRegistrationPlugin.cpp \
	RedirectionManager.cpp \
	JITLinkRedirectableSymbolManager.cpp \
	ReOptimizeLayer.cpp

.include <bsd.lib.mk>
