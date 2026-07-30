#	$NetBSD: Makefile,v 1.3 2024/07/02 21:56:00 mrg Exp $

LIB=	LLVMAMDGPUCodeGen

.include <bsd.init.mk>

CPPFLAGS+=	-I${LLVM_SRCDIR}/lib/Target/AMDGPU

.PATH: ${LLVM_SRCDIR}/lib/Target/AMDGPU

SRCS+=	AMDGPUAliasAnalysis.cpp \
	AMDGPUAlwaysInlinePass.cpp \
	AMDGPUAnnotateUniformValues.cpp \
	AMDGPUArgumentUsageInfo.cpp \
	AMDGPUAsanInstrumentation.cpp \
	AMDGPUAsmPrinter.cpp \
	AMDGPUAtomicOptimizer.cpp \
	AMDGPUAttributor.cpp \
	AMDGPUBarrierLatency.cpp \
	AMDGPUCallLowering.cpp \
	AMDGPUCodeGenPrepare.cpp \
	AMDGPUCombinerHelper.cpp \
	AMDGPUCtorDtorLowering.cpp \
	AMDGPUExportClustering.cpp \
	AMDGPUExportKernelRuntimeHandles.cpp \
	AMDGPUFrameLowering.cpp \
	AMDGPUGlobalISelDivergenceLowering.cpp \
	AMDGPUGlobalISelUtils.cpp \
	AMDGPUHSAMetadataStreamer.cpp \
	AMDGPUInsertDelayAlu.cpp \
	AMDGPUInstCombineIntrinsic.cpp \
	AMDGPUUniformIntrinsicCombine.cpp \
	AMDGPUInstrInfo.cpp \
	AMDGPUInstructionSelector.cpp \
	AMDGPUISelDAGToDAG.cpp \
	AMDGPUISelLowering.cpp \
	AMDGPULateCodeGenPrepare.cpp \
	AMDGPULegalizerInfo.cpp \
	AMDGPULibCalls.cpp \
	AMDGPUImageIntrinsicOptimizer.cpp \
	AMDGPULibFunc.cpp \
	AMDGPULowerBufferFatPointers.cpp \
	AMDGPULowerIntrinsics.cpp \
	AMDGPULowerKernelArguments.cpp \
	AMDGPULowerKernelAttributes.cpp \
	AMDGPULowerModuleLDSPass.cpp \
	AMDGPUPrepareAGPRAlloc.cpp \
	AMDGPULowerExecSync.cpp \
	AMDGPUSwLowerLDS.cpp \
	AMDGPUMachineFunction.cpp \
	AMDGPUMachineModuleInfo.cpp \
	AMDGPUMacroFusion.cpp \
	AMDGPUMCInstLower.cpp \
	AMDGPUMemoryUtils.cpp \
	AMDGPUIGroupLP.cpp \
	AMDGPULowerVGPREncoding.cpp \
	AMDGPUMCResourceInfo.cpp \
	AMDGPUMarkLastScratchLoad.cpp \
	AMDGPUMIRFormatter.cpp \
	AMDGPUPerfHintAnalysis.cpp \
	AMDGPUPostLegalizerCombiner.cpp \
	AMDGPUPreLegalizerCombiner.cpp \
	AMDGPUPreloadKernArgProlog.cpp \
	AMDGPUPreloadKernelArguments.cpp \
	AMDGPUPrintfRuntimeBinding.cpp \
	AMDGPUPromoteAlloca.cpp \
	AMDGPUPromoteKernelArguments.cpp \
	AMDGPURegBankCombiner.cpp \
	AMDGPURegBankLegalize.cpp \
	AMDGPURegBankLegalizeHelper.cpp \
	AMDGPURegBankLegalizeRules.cpp \
	AMDGPURegBankSelect.cpp \
	AMDGPURegisterBankInfo.cpp \
	AMDGPURemoveIncompatibleFunctions.cpp \
	AMDGPUReserveWWMRegs.cpp \
	AMDGPUResourceUsageAnalysis.cpp \
	AMDGPURewriteAGPRCopyMFMA.cpp \
	AMDGPURewriteOutArguments.cpp \
	AMDGPURewriteUndefForPHI.cpp \
	AMDGPUSelectionDAGInfo.cpp \
	AMDGPUSetWavePriority.cpp \
	AMDGPUSplitModule.cpp \
	AMDGPUSubtarget.cpp \
	AMDGPUTargetMachine.cpp \
	AMDGPUTargetObjectFile.cpp \
	AMDGPUTargetTransformInfo.cpp \
	AMDGPUWaitSGPRHazards.cpp \
	AMDGPUUnifyDivergentExitNodes.cpp \
	R600MachineCFGStructurizer.cpp \
	GCNCreateVOPD.cpp \
	GCNDPPCombine.cpp \
	GCNHazardRecognizer.cpp \
	GCNILPSched.cpp \
	GCNIterativeScheduler.cpp \
	GCNMinRegStrategy.cpp \
	GCNNSAReassign.cpp \
	GCNPreRAOptimizations.cpp \
	GCNPreRALongBranchReg.cpp \
	GCNRegPressure.cpp \
	GCNRewritePartialRegUses.cpp \
	GCNSchedStrategy.cpp \
	GCNSubtarget.cpp \
	GCNVOPDUtils.cpp \
	R600AsmPrinter.cpp \
	R600ClauseMergePass.cpp \
	R600ControlFlowFinalizer.cpp \
	R600EmitClauseMarkers.cpp \
	R600ExpandSpecialInstrs.cpp \
	R600FrameLowering.cpp \
	R600InstrInfo.cpp \
	R600ISelDAGToDAG.cpp \
	R600ISelLowering.cpp \
	R600MachineFunctionInfo.cpp \
	R600MachineScheduler.cpp \
	R600MCInstLower.cpp \
	R600OpenCLImageTypeLoweringPass.cpp \
	R600OptimizeVectorRegisters.cpp \
	R600Packetizer.cpp \
	R600RegisterInfo.cpp \
	R600Subtarget.cpp \
	R600TargetMachine.cpp \
	R600TargetTransformInfo.cpp \
	SIAnnotateControlFlow.cpp \
	SIFixSGPRCopies.cpp \
	SIFixVGPRCopies.cpp \
	SIFoldOperands.cpp \
	SIFormMemoryClauses.cpp \
	SIFrameLowering.cpp \
	SIInsertHardClauses.cpp \
	SIInsertWaitcnts.cpp \
	SIInstrInfo.cpp \
	SIISelLowering.cpp \
	SILateBranchLowering.cpp \
	SILoadStoreOptimizer.cpp \
	SILowerControlFlow.cpp \
	SILowerI1Copies.cpp \
	SILowerWWMCopies.cpp \
	SILowerSGPRSpills.cpp \
	SIMachineFunctionInfo.cpp \
	SIMachineScheduler.cpp \
	SIMemoryLegalizer.cpp \
	SIModeRegister.cpp \
	SIModeRegisterDefaults.cpp \
	SIOptimizeExecMasking.cpp \
	SIOptimizeExecMaskingPreRA.cpp \
	SIOptimizeVGPRLiveRange.cpp \
	SIPeepholeSDWA.cpp \
	SIPostRABundler.cpp \
	SIPreAllocateWWMRegs.cpp \
	SIPreEmitPeephole.cpp \
	SIProgramInfo.cpp \
	SIRegisterInfo.cpp \
	SIShrinkInstructions.cpp \
	SIWholeQuadMode.cpp

TABLEGEN_SRC=		AMDGPU.td AMDGPUGISel.td R600.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/AMDGPU
TABLEGEN_OUTPUT.AMDGPU.td= \
	AMDGPUGenAsmMatcher.inc|-gen-asm-matcher \
	AMDGPUGenAsmWriter.inc|-gen-asm-writer \
	AMDGPUGenCallingConv.inc|-gen-callingconv \
	AMDGPUGenDAGISel.inc|-gen-dag-isel \
	AMDGPUGenDisassemblerTables.inc|-gen-disassembler^--specialize-decoders-per-bitwidth^-ignore-non-decodable-operands^-ignore-fully-defined-operands \
	AMDGPUGenInstrInfo.inc|-gen-instr-info \
	AMDGPUGenMCCodeEmitter.inc|-gen-emitter \
	AMDGPUGenMCPseudoLowering.inc|-gen-pseudo-lowering \
	AMDGPUGenRegisterBank.inc|-gen-register-bank \
	AMDGPUGenRegisterInfo.inc|-gen-register-info \
	AMDGPUGenSDNodeInfo.inc|-gen-sd-node-info \
	AMDGPUGenSearchableTables.inc|-gen-searchable-tables \
	AMDGPUGenSubtargetInfo.inc|-gen-subtarget

TABLEGEN_OUTPUT.AMDGPUGISel.td= \
	AMDGPUGenGlobalISel.inc|-gen-global-isel \
	AMDGPUGenPostLegalizeGICombiner.inc|-gen-global-isel-combiner^-combiners=AMDGPUPostLegalizerCombiner \
	AMDGPUGenPreLegalizeGICombiner.inc|-gen-global-isel-combiner^-combiners=AMDGPUPreLegalizerCombiner \
	AMDGPUGenRegBankGICombiner.inc|-gen-global-isel-combiner^-combiners=AMDGPURegBankCombiner

TABLEGEN_OUTPUT.R600.td= \
	R600GenAsmWriter.inc|-gen-asm-writer \
	R600GenCallingConv.inc|-gen-callingconv \
	R600GenDAGISel.inc|-gen-dag-isel \
	R600GenDFAPacketizer.inc|-gen-dfa-packetizer \
	R600GenInstrInfo.inc|-gen-instr-info \
	R600GenMCCodeEmitter.inc|-gen-emitter \
	R600GenRegisterInfo.inc|-gen-register-info \
	R600GenSubtargetInfo.inc|-gen-subtarget

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif

CWARNFLAGS.gcc+=  ${CC_WNO_STRINGOP_OVERREAD}
