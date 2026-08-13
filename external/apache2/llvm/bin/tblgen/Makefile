#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:49 joerg Exp $

PROG_CXX=	llvm-tblgen
NOMAN=		yes

.include <bsd.init.mk>

CPPFLAGS+=	-I${LLVM_SRCDIR}/utils/TableGen

.PATH: ${LLVM_SRCDIR}/utils/TableGen

SRCS=	AsmMatcherEmitter.cpp \
	AsmWriterEmitter.cpp \
	CallingConvEmitter.cpp \
	CodeEmitterGen.cpp \
	CodeGenMapTable.cpp \
	CompressInstEmitter.cpp \
	CTagsEmitter.cpp \
	DAGISelEmitter.cpp \
	DAGISelMatcher.cpp \
	DAGISelMatcherEmitter.cpp \
	DAGISelMatcherGen.cpp \
	DAGISelMatcherOpt.cpp \
	DecoderEmitter.cpp \
	DecoderTableEmitter.cpp \
	DecoderTree.cpp \
	DFAEmitter.cpp \
	DFAPacketizerEmitter.cpp \
	DisassemblerEmitter.cpp \
	DXILEmitter.cpp \
	ExegesisEmitter.cpp \
	FastISelEmitter.cpp \
	GlobalISelCombinerEmitter.cpp \
	GlobalISelEmitter.cpp \
	InstrDocsEmitter.cpp \
	InstrInfoEmitter.cpp \
	llvm-tblgen.cpp \
	MacroFusionPredicatorEmitter.cpp \
	OptionParserEmitter.cpp \
	OptionRSTEmitter.cpp \
	PseudoLoweringEmitter.cpp \
	RegisterBankEmitter.cpp \
	RegisterInfoEmitter.cpp \
	SDNodeInfoEmitter.cpp \
	SearchableTableEmitter.cpp \
	SubtargetEmitter.cpp \
	WebAssemblyDisassemblerEmitter.cpp \
	X86InstrMappingEmitter.cpp \
	X86DisassemblerTables.cpp \
	X86FoldTablesEmitter.cpp \
	X86MnemonicTables.cpp \
	X86ModRMFilters.cpp \
	X86RecognizableInstr.cpp

.if defined(HOSTPROG)
LLVM_LIBS_WHOLE_ARCHIVE+= \
	TableGenCommon \
	TableGenBasic

LLVM_LIBS+= \
	TableGen
.else
LLVM_LIBS_STATIC_WHOLE_ARCHIVE+= \
	TableGenCommon \
	TableGenBasic

LLVM_LIBS_STATIC+= \
	TableGen
.endif

LLVM_LIBS+= \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.if defined(HOSTPROG)
CPPFLAGS+=	-I${LLVM_OBJDIR.TableGenCommon}
.endif

.include <bsd.prog.mk>
