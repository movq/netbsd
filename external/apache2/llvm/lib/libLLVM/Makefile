# $NetBSD$

LIB=	LLVM
LIBISCXX=	yes

.include <bsd.init.mk>

LIBISPRIVATE=	no

.include "${.PARSEDIR}/../../Makefile.inc"
.include "${.PARSEDIR}/../../dylib.mk"

.PATH: ${LLVM_SRCDIR}/tools/llvm-shlib

SRCS=	libllvm.cpp

SHLIB_MAJOR=		${LLVM_SHLIB_MAJOR}
SHLIB_MINOR=		${LLVM_SHLIB_MINOR}
VERSION_MAP=		LLVM.map

.for l in ${LLVM_DYLIB_COMPONENTS}
LLVM_COMPONENT_OBJDIR.${l}!=	cd ${LLVM_TOPLEVEL}/lib/libLLVM${l} && ${PRINTOBJDIR}
LLVM_COMPONENT_ARCHIVES+=	${LLVM_COMPONENT_OBJDIR.${l}}/libLLVM${l}_pic.a
.endfor

DPADD+=	${LLVM_COMPONENT_ARCHIVES}
LDADD+=	-Wl,--whole-archive ${LLVM_COMPONENT_ARCHIVES} -Wl,--no-whole-archive

LDADD+=	-lz -lexecinfo -lterminfo -lpthread -lrt
DPADD+=	${LIBZ} ${LIBEXECINFO} ${LIBTERMINFO} ${LIBRT}

LDFLAGS+=	-Wl,-z,defs -Wl,-Bsymbolic-functions

.include <bsd.lib.mk>
