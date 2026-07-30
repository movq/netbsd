#	$NetBSD$

LIB=	clangOptions
.include <bsd.init.mk>
.PATH: ${CLANG_SRCDIR}/lib/Options
SRCS+=	DriverOptions.cpp \
	OptionUtils.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
