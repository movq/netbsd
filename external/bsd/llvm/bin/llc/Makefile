#	$NetBSD: Makefile,v 1.12 2013/01/23 15:02:54 joerg Exp $

PROG_CXX=	llc
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llc

SRCS=	llc.cpp

LLVM_LIBS+= \
	AsmPrinter \
	AsmParser \
	BitReader \
	ARMCodeGen \
	ARMTargetInfo \
	ARMMCTargetDesc \
	ARMAsmParser \
	ARMAsmPrinter \
	MipsCodeGen \
	MipsTargetInfo \
	MipsAsmParser \
	MipsMCTargetDesc \
	MipsAsmPrinter \
	PowerPCCodeGen \
	PowerPCTargetInfo \
	PowerPCMCTargetDesc \
	PowerPCAsmPrinter \
	SparcCodeGen \
	SparcTargetInfo \
	SparcMCTargetDesc \
	X86CodeGen \
	X86TargetInfo \
	X86MCTargetDesc \
	X86AsmParser \
	X86AsmPrinter \
	X86Utils \
	SelectionDAG \
	CodeGen \
	ScalarOpts \
	TransformsUtils \
	Analysis \
	Target \
	MCParser \
	MC \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
