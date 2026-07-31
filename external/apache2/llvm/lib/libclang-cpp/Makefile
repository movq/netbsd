# $NetBSD$

LIB=	clang-cpp
LIBISCXX=	yes

.include <bsd.init.mk>

LIBISPRIVATE=	no

.include "${.PARSEDIR}/../../Makefile.inc"
.include "${.PARSEDIR}/../../dylib.mk"

.PATH: ${CLANG_SRCDIR}/tools/clang-shlib

SRCS=	clang-shlib.cpp

SHLIB_MAJOR=		${LLVM_SHLIB_MAJOR}
SHLIB_MINOR=		${LLVM_SHLIB_MINOR}
VERSION_MAP=		clang-cpp.map

.for l in ${CLANG_DYLIB_COMPONENTS}
CLANG_COMPONENT_OBJDIR.${l}!=	cd ${LLVM_TOPLEVEL}/lib/lib${l} && ${PRINTOBJDIR}
CLANG_COMPONENT_ARCHIVES+=	${CLANG_COMPONENT_OBJDIR.${l}}/lib${l}_pic.a
.endfor

LLVM_SHLIB_OBJDIR!=	cd ${LLVM_TOPLEVEL}/lib/libLLVM && ${PRINTOBJDIR}

DPADD+=	${CLANG_COMPONENT_ARCHIVES}
LDADD+=	-Wl,--whole-archive ${CLANG_COMPONENT_ARCHIVES} -Wl,--no-whole-archive
LDADD+=	-L${LLVM_SHLIB_OBJDIR} -lLLVM
DPADD+=	${LLVM_SHLIB_OBJDIR}/libLLVM.so.${LLVM_SHLIB_FULLVERSION}

LDFLAGS+=	-Wl,-z,defs -Wl,-Bsymbolic-functions

.include <bsd.lib.mk>
