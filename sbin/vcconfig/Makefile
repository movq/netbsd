#	$NetBSD$

PROG=	vcconfig
SRCS=	vcconfig.c veracrypt.c

LDADD=	-lcrypto -lutil
DPADD=	${LIBCRYPTO} ${LIBUTIL}

MAN=	vcconfig.8

.include <bsd.prog.mk>
