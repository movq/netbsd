#	$NetBSD: Makefile,v 1.8 2021/05/30 01:56:59 joerg Exp $

.include <bsd.init.mk>

.PATH: ${LIBCXX_SRCDIR}/include

INCSDIR=	/usr/include/c++

.include "${.PARSEDIR}/Makefile.headers"

INCS+=	__assertion_handler __config_site module.modulemap cxxabi.h

__assertion_handler: ${LIBCXX_SRCDIR}/vendor/llvm/default_assertion_handler.in
	cp ${.ALLSRC} ${.TARGET}

module.modulemap: ${LIBCXX_SRCDIR}/include/module.modulemap.in
	sed -e 's,@LIBCXX_CONFIG_SITE_MODULE_ENTRY@,textual header "__config_site",' \
	    ${.ALLSRC} > ${.TARGET}


cxxabi.h: ${LIBCXXRT_SRCDIR}/src/cxxabi.h
	cp ${LIBCXXRT_SRCDIR}/src/cxxabi.h .

DPSRCS+=	cxxabi.h
CLEANFILES+=	cxxabi.h

DPSRCS+=	__assertion_handler module.modulemap
CLEANFILES+=	__assertion_handler module.modulemap

realall: ${DPSRCS}

.include <bsd.clean.mk>
.include <bsd.inc.mk>
.include <bsd.obj.mk>
