#	$NetBSD: Makefile,v 1.4 2021/05/30 01:56:55 joerg Exp $

LIB=	clangAST

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/AST

SRCS+=	APValue.cpp \
	ASTConcept.cpp \
	ASTConsumer.cpp \
	ASTContext.cpp \
	ASTDiagnostic.cpp \
	ASTDumper.cpp \
	ASTImporter.cpp \
	ASTImporterLookupTable.cpp \
	ASTStructuralEquivalence.cpp \
	ASTTypeTraits.cpp \
	AttrDocTable.cpp \
	AttrImpl.cpp \
	Availability.cpp \
	CommentBriefParser.cpp \
	CommentCommandTraits.cpp \
	Comment.cpp \
	CommentLexer.cpp \
	CommentParser.cpp \
	CommentSema.cpp \
	ComparisonCategories.cpp \
	ComputeDependence.cpp \
	CXXInheritance.cpp \
	DataCollection.cpp \
	DeclarationName.cpp \
	DynamicRecursiveASTVisitor.cpp \
	DeclBase.cpp \
	Decl.cpp \
	DeclCXX.cpp \
	DeclFriend.cpp \
	DeclGroup.cpp \
	DeclObjC.cpp \
	DeclOpenACC.cpp \
	DeclOpenMP.cpp \
	DeclPrinter.cpp \
	DeclTemplate.cpp \
	ExprClassification.cpp \
	ExprConcepts.cpp \
	ExprConstant.cpp \
	Expr.cpp \
	ExprCXX.cpp \
	ExprObjC.cpp \
	ExternalASTMerger.cpp \
	ExternalASTSource.cpp \
	FormatString.cpp \
	InferAlloc.cpp \
	InheritViz.cpp \
	ItaniumCXXABI.cpp \
	ItaniumMangle.cpp \
	JSONNodeDumper.cpp \
	Mangle.cpp \
	MicrosoftCXXABI.cpp \
	MicrosoftMangle.cpp \
	NestedNameSpecifier.cpp \
	NSAPI.cpp \
	ODRDiagsEmitter.cpp \
	ODRHash.cpp \
	OpenACCClause.cpp \
	OpenMPClause.cpp \
	OSLog.cpp \
	ParentMapContext.cpp \
	ParentMap.cpp \
	PrintfFormatString.cpp \
	QualTypeNames.cpp \
	Randstruct.cpp \
	RawCommentList.cpp \
	RecordLayoutBuilder.cpp \
	RecordLayout.cpp \
	ScanfFormatString.cpp \
	SelectorLocationsKind.cpp \
	Stmt.cpp \
	StmtCXX.cpp \
	StmtIterator.cpp \
	StmtObjC.cpp \
	StmtOpenACC.cpp \
	StmtOpenMP.cpp \
	StmtPrinter.cpp \
	StmtProfile.cpp \
	StmtViz.cpp \
	TemplateBase.cpp \
	TemplateName.cpp \
	TextNodeDumper.cpp \
	Type.cpp \
	TypeLoc.cpp \
	TypePrinter.cpp \
	VTableBuilder.cpp \
	VTTBuilder.cpp

.PATH: ${CLANG_SRCDIR}/lib/AST/ByteCode

SRCS+=	BitcastBuffer.cpp \
	ByteCodeEmitter.cpp \
	Compiler.cpp \
	Context.cpp \
	Descriptor.cpp \
	Disasm.cpp \
	DynamicAllocator.cpp \
	EvalEmitter.cpp \
	EvaluationResult.cpp \
	Floating.cpp \
	Function.cpp \
	FunctionPointer.cpp \
	InitMap.cpp \
	Interp.cpp \
	InterpBlock.cpp \
	InterpBuiltin.cpp \
	InterpBuiltinBitCast.cpp \
	InterpFrame.cpp \
	InterpShared.cpp \
	InterpStack.cpp \
	InterpState.cpp \
	MemberPointer.cpp \
	Pointer.cpp \
	PrimType.cpp \
	Program.cpp \
	Record.cpp \
	Source.cpp \
	State.cpp

CLANG_TABLEGEN_SRC=	Opcodes.td
CLANG_TABLEGEN_OUTPUT.Opcodes.td= \
	Opcodes.inc|-gen-clang-opcodes

.PATH: ${CLANG_SRCDIR}/include/clang/Basic

CLANG_TABLEGEN_SRC+=	Attr.td
CLANG_TABLEGEN_INCLUDES.Attr.td=	-I${CLANG_SRCDIR}/include
CLANG_TABLEGEN_OUTPUT.Attr.td= \
	AttrDocTable.inc|-gen-clang-attr-doc-table

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
