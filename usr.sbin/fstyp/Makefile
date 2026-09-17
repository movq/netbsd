#	$NetBSD: Makefile,v 1.15 2024/04/26 18:21:25 christos Exp $

.include <bsd.own.mk>

PROG=	fstyp
SRCS=	apfs.c cd9660.c exfat.c ext2fs.c fstyp.c hammer.c hammer2.c hfsplus.c msdosfs.c ntfs.c ufs.c

# ZFS probing is deferred until it is integrated with OpenZFS.

MAN=	fstyp.8

WARNS?=	6

CWARNFLAGS.clang+=	-Wno-unknown-pragmas
CWARNFLAGS+=        	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
