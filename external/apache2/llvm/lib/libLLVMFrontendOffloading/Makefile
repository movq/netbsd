#	$NetBSD$

LIB=	LLVMFrontendOffloading
.include <bsd.init.mk>
.PATH: ${LLVM_SRCDIR}/lib/Frontend/Offloading
SRCS+=	Utility.cpp \
	OffloadWrapper.cpp \
	PropertySet.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
