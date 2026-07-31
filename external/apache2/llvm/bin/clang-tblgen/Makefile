#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:46 joerg Exp $

PROG_CXX=	clang-tblgen
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/utils/TableGen

SRCS=	ASTTableGen.cpp \
	CIRLoweringEmitter.cpp \
	ClangASTNodesEmitter.cpp \
	ClangASTPropertiesEmitter.cpp \
	ClangAttrEmitter.cpp \
	ClangBuiltinsEmitter.cpp \
	ClangBuiltinTemplatesEmitter.cpp \
	ClangCommentCommandInfoEmitter.cpp \
	ClangCommentHTMLNamedCharacterReferenceEmitter.cpp \
	ClangCommentHTMLTagsEmitter.cpp \
	ClangDataCollectorsEmitter.cpp \
	ClangDiagnosticsEmitter.cpp \
	ClangOpcodesEmitter.cpp \
	ClangOpenCLBuiltinEmitter.cpp \
	ClangOptionDocEmitter.cpp \
	ClangSACheckersEmitter.cpp \
	ClangSyntaxEmitter.cpp \
	ClangTypeNodesEmitter.cpp \
	MveEmitter.cpp \
	NeonEmitter.cpp \
	RISCVVEmitter.cpp \
	SveEmitter.cpp \
	TableGen.cpp

CLANG_LIBS+= \
	clangSupport

.if defined(HOSTPROG)
LLVM_LIBS+= \
	TableGen
.else
LLVM_LIBS_STATIC+= \
	TableGen
.endif

LLVM_LIBS+= \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
