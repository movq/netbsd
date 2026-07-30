#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:09 joerg Exp $

LIB=	LLVMMipsCodeGen

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/Mips

SRCS+=	Mips16FrameLowering.cpp \
	Mips16HardFloat.cpp \
	Mips16HardFloatInfo.cpp \
	Mips16InstrInfo.cpp \
	Mips16ISelDAGToDAG.cpp \
	Mips16ISelLowering.cpp \
	Mips16RegisterInfo.cpp \
	MipsAnalyzeImmediate.cpp \
	MipsAsmPrinter.cpp \
	MipsCallLowering.cpp \
	MipsCCState.cpp \
	MipsConstantIslandPass.cpp \
	MipsDelaySlotFiller.cpp \
	MipsExpandPseudo.cpp \
	MipsFastISel.cpp \
	MipsInstrInfo.cpp \
	MipsInstructionSelector.cpp \
	MipsISelDAGToDAG.cpp \
	MipsISelLowering.cpp \
	MipsFrameLowering.cpp \
	MipsLegalizerInfo.cpp \
	MipsBranchExpansion.cpp \
	MipsMCInstLower.cpp \
	MipsMachineFunction.cpp \
	MipsModuleISelDAGToDAG.cpp \
	MipsOptimizePICCall.cpp \
	MipsOs16.cpp \
	MipsPreLegalizerCombiner.cpp \
	MipsPostLegalizerCombiner.cpp \
	MipsRegisterBankInfo.cpp \
	MipsRegisterInfo.cpp \
	MipsSEFrameLowering.cpp \
	MipsSEInstrInfo.cpp \
	MipsSEISelDAGToDAG.cpp \
	MipsSEISelLowering.cpp \
	MipsSERegisterInfo.cpp \
	MipsSelectionDAGInfo.cpp \
	MipsSubtarget.cpp \
	MipsTargetMachine.cpp \
	MipsTargetObjectFile.cpp \
	MipsTargetTransformInfo.cpp \
	MicroMipsSizeReduction.cpp \
	MipsMulMulBugPass.cpp

CPPFLAGS+=-I${LLVM_SRCDIR}/lib/Target/Mips

TABLEGEN_SRC=		Mips.td
TABLEGEN_INCLUDES=	-I${LLVM_SRCDIR}/lib/Target/Mips
TABLEGEN_OUTPUT= \
	MipsGenAsmMatcher.inc|-gen-asm-matcher \
	MipsGenAsmWriter.inc|-gen-asm-writer \
	MipsGenCallingConv.inc|-gen-callingconv \
	MipsGenDAGISel.inc|-gen-dag-isel \
	MipsGenDisassemblerTables.inc|-gen-disassembler^-ignore-non-decodable-operands \
	MipsGenFastISel.inc|-gen-fast-isel \
	MipsGenGlobalISel.inc|-gen-global-isel \
	MipsGenPostLegalizeGICombiner.inc|-gen-global-isel-combiner^-combiners=MipsPostLegalizerCombiner \
	MipsGenInstrInfo.inc|-gen-instr-info \
	MipsGenMCCodeEmitter.inc|-gen-emitter \
	MipsGenMCPseudoLowering.inc|-gen-pseudo-lowering \
	MipsGenRegisterBank.inc|-gen-register-bank \
	MipsGenRegisterInfo.inc|-gen-register-info \
	MipsGenSDNodeInfo.inc|-gen-sd-node-info \
	MipsGenSubtargetInfo.inc|-gen-subtarget \
	MipsGenExegesis.inc|-gen-exegesis

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
