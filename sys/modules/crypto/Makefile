#	$NetBSD: Makefile,v 1.4.16.1 2018/09/23 01:33:26 pgoyette Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DCOMPAT_50

.PATH:	${S}/opencrypto

KMOD=	crypto
IOCONF=	crypto.ioconf
SRCS=	cryptodev.c

.include <bsd.kmodule.mk>
