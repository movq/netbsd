LIB=    LLVMTableGenCommon

.include <bsd.init.mk>

.if defined(HOSTLIB)
genvt_inc=	${.OBJDIR}/llvm/CodeGen/GenVT.inc

CPPFLAGS+=	-I${.OBJDIR}

${genvt_inc}:	${TOOLDIR}/bin/${_TOOL_PREFIX}llvm-tblgen-min
	mkdir -p ${genvt_inc:H}
	${TOOLDIR}/bin/${_TOOL_PREFIX}llvm-tblgen-min -gen-vt \
	    ${LLVM_SRCDIR}/include/llvm/CodeGen/ValueTypes.td \
	    -o ${genvt_inc}

DPSRCS+=	${genvt_inc}
CLEANFILES+=	${genvt_inc}
.endif

CPPFLAGS+=	-I${LLVM_SRCDIR}/utils/TableGen

.PATH: ${LLVM_SRCDIR}/utils/TableGen/Common

SRCS+=	AsmWriterInst.cpp \
	CodeGenDAGPatterns.cpp \
	CodeGenHwModes.cpp \
	CodeGenInstAlias.cpp \
	CodeGenInstruction.cpp \
	CodeGenRegisters.cpp \
	CodeGenSchedule.cpp \
	CodeGenTarget.cpp \
	InfoByHwMode.cpp \
	InstructionEncoding.cpp \
	OptEmitter.cpp \
	PredicateExpander.cpp \
	SubtargetFeatureInfo.cpp \
	Types.cpp \
	Utils.cpp \
	VarLenCodeEmitterGen.cpp

.PATH: ${LLVM_SRCDIR}/utils/TableGen/Common/GlobalISel
SRCS+=	CodeExpander.cpp \
	CombinerUtils.cpp \
	CXXPredicates.cpp \
	GlobalISelMatchTable.cpp \
	GlobalISelMatchTableExecutorEmitter.cpp \
	PatternParser.cpp \
	Patterns.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
