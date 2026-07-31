#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:49 joerg Exp $

PROG_CXX=	llvm-tblgen-min
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/utils/TableGen

SRCS=	llvm-min-tblgen.cpp

.if defined(HOSTPROG)
LLVM_LIBS_WHOLE_ARCHIVE+= \
	TableGenBasic

LLVM_LIBS+= \
	TableGen
.else
LLVM_LIBS_STATIC_WHOLE_ARCHIVE+= \
	TableGenBasic

LLVM_LIBS_STATIC+= \
	TableGen
.endif

LLVM_LIBS+= \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
