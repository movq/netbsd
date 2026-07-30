#	$NetBSD$

LIB=	clangAnalysisLifetimeSafety
.include <bsd.init.mk>
.PATH: ${CLANG_SRCDIR}/lib/Analysis/LifetimeSafety
SRCS+=	Checker.cpp \
	Facts.cpp \
	FactsGenerator.cpp \
	LifetimeAnnotations.cpp \
	LifetimeSafety.cpp \
	LiveOrigins.cpp \
	LifetimeStats.cpp \
	Loans.cpp \
	LoanPropagation.cpp \
	Origins.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
