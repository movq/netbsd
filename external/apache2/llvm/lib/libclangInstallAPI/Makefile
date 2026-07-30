#	$NetBSD$

LIB=	clangInstallAPI

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/InstallAPI

SRCS+=	DiagnosticBuilderWrappers.cpp \
	DirectoryScanner.cpp \
	DylibVerifier.cpp \
	FileList.cpp \
	Frontend.cpp \
	HeaderFile.cpp \
	Library.cpp \
	Visitor.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
