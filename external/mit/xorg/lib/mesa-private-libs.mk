#	$NetBSD$

# Mesa utility code is linked privately into each frontend DSO upstream.
# Keep xmlconfig before mesa_util so the latter can satisfy its references.

MESA_XMLCONFIG_OBJDIR!=	cd ${.CURDIR}/../libxmlconfig && ${PRINTOBJDIR}
MESA_UTIL_OBJDIR!=	cd ${.CURDIR}/../libmesa_util && ${PRINTOBJDIR}

DPADD+=	${MESA_XMLCONFIG_OBJDIR}/libxmlconfig_pic.a
DPADD+=	${MESA_UTIL_OBJDIR}/libmesa_util_pic.a
LDADD+=	${MESA_XMLCONFIG_OBJDIR}/libxmlconfig_pic.a
LDADD+=	${MESA_UTIL_OBJDIR}/libmesa_util_pic.a
