#
# a makefile for /usr/bin/rwall
#
# $Header: /home/mike/src/cvs/netbsd/src/usr.bin/rwall/Makefile,v 1.2 1993/06/03 19:32:37 brezak Exp $
#

PROG = rwall
NOMAN=	noman

DPADD=	${LIBRPCSVC} ${LIBRPC}
LDADD=	${LDARGS} -lrpcsvc -lrpc

.include <bsd.prog.mk>
