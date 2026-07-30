#	$NetBSD$

LIB=	LLVMCGData
.include <bsd.init.mk>
.PATH: ${LLVM_SRCDIR}/lib/CGData
SRCS+=	CodeGenData.cpp \
	CodeGenDataReader.cpp \
	CodeGenDataWriter.cpp \
	OutlinedHashTree.cpp \
	OutlinedHashTreeRecord.cpp \
	StableFunctionMap.cpp \
	StableFunctionMapRecord.cpp
.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
