# $NetBSD$

.include <bsd.init.mk>
LIB=	spl
SRCS=	assert.c backtrace.c getexecname.c list.c mkdirp.c page.c
SRCS+=	timestamp.c tunables.c getexecname_os.c
SRCS+=	mnttab.c zone.c gethostid.c
.PATH:	${ZFSDIR}/lib/libspl ${ZFSDIR}/lib/libspl/os/netbsd
.include <bsd.lib.mk>
