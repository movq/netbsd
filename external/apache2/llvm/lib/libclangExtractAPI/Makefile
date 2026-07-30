#	$NetBSD$

LIB=	clangExtractAPI

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/ExtractAPI
.PATH: ${CLANG_SRCDIR}/lib/ExtractAPI/Serialization

SRCS+=	API.cpp \
	APIIgnoresList.cpp \
	ExtractAPIConsumer.cpp \
	DeclarationFragments.cpp \
	SymbolGraphSerializer.cpp \
	TypedefUnderlyingTypeResolver.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
