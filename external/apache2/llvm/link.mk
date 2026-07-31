#	$NetBSD: link.mk,v 1.2 2021/05/30 01:56:45 joerg Exp $

.include <bsd.own.mk>

LLVM_TOPLEVEL:=	${.PARSEDIR}

.if defined(HOSTPROG)
LIB_BASE=	${NETBSDSRCDIR}/tools/llvm-lib

.for l in ${CLANG_LIBS}
CLANG_OBJDIR.${l}!=	cd ${LIB_BASE}/lib${l} && ${PRINTOBJDIR}
LDADD+=	-L${CLANG_OBJDIR.${l}} -l${l}
DPADD+=	${CLANG_OBJDIR.${l}}/lib${l}.a
.endfor

.for l in ${LLVM_LIBS_WHOLE_ARCHIVE}
LLVM_OBJDIR.${l}!=	cd ${LIB_BASE}/libLLVM${l} && ${PRINTOBJDIR}
LDADD+=	-Wl,--whole-archive -L${LLVM_OBJDIR.${l}} -lLLVM${l} -Wl,--no-whole-archive
DPADD+=	${LLVM_OBJDIR.${l}}/libLLVM${l}.a
.endfor

.for l in ${LLVM_LIBS}
LLVM_OBJDIR.${l}!=	cd ${LIB_BASE}/libLLVM${l} && ${PRINTOBJDIR}
LDADD+=	-L${LLVM_OBJDIR.${l}} -lLLVM${l}
DPADD+=	${LLVM_OBJDIR.${l}}/libLLVM${l}.a
.endfor

.else
.include "${LLVM_TOPLEVEL}/dylib.mk"

LLVM_SHLIB_OBJDIR!=	cd ${LLVM_TOPLEVEL}/lib/libLLVM && ${PRINTOBJDIR}
CLANG_SHLIB_OBJDIR!=	cd ${LLVM_TOPLEVEL}/lib/libclang-cpp && ${PRINTOBJDIR}

# TableGen is deliberately excluded from libLLVM.  Target TableGen tools link
# those few build-only components statically and use the shared libraries for
# everything else.
.for l in ${LLVM_LIBS_STATIC_WHOLE_ARCHIVE}
LLVM_STATIC_OBJDIR.${l}!=	cd ${LLVM_TOPLEVEL}/lib/libLLVM${l} && ${PRINTOBJDIR}
LDADD+=	-Wl,--whole-archive \
	${LLVM_STATIC_OBJDIR.${l}}/libLLVM${l}_pic.a \
	-Wl,--no-whole-archive
DPADD+=	${LLVM_STATIC_OBJDIR.${l}}/libLLVM${l}_pic.a
.endfor

.for l in ${LLVM_LIBS_STATIC}
LLVM_STATIC_OBJDIR.${l}!=	cd ${LLVM_TOPLEVEL}/lib/libLLVM${l} && ${PRINTOBJDIR}
LDADD+=	${LLVM_STATIC_OBJDIR.${l}}/libLLVM${l}_pic.a
DPADD+=	${LLVM_STATIC_OBJDIR.${l}}/libLLVM${l}_pic.a
.endfor

.if !empty(CLANG_LIBS)
LDADD+=	-L${CLANG_SHLIB_OBJDIR} -lclang-cpp
DPADD+=	${CLANG_SHLIB_OBJDIR}/libclang-cpp.so.${LLVM_SHLIB_FULLVERSION}
.endif

.if !empty(LLVM_LIBS) || !empty(LLVM_LIBS_WHOLE_ARCHIVE) || \
    !empty(CLANG_LIBS)
LDADD+=	-L${LLVM_SHLIB_OBJDIR} -lLLVM
DPADD+=	${LLVM_SHLIB_OBJDIR}/libLLVM.so.${LLVM_SHLIB_FULLVERSION}
.endif
.endif

.if defined(HOSTPROG)
LDADD_NEED_DL=		cat ${LLVM_TOOLCONF_OBJDIR}/need-dl 2> /dev/null || true
LDADD_NEED_TERMINFO=	cat ${LLVM_TOOLCONF_OBJDIR}/need-terminfo 2> /dev/null || true
LDADD+=	${LDADD_NEED_DL:sh} ${LDADD_NEED_TERMINFO:sh}
.else
LDADD+=	-lterminfo
DPADD+=	${LIBTERMINFO}
.endif

LDADD+=	-lpthread
