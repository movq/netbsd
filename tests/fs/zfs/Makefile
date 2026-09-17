#	$NetBSD: Makefile,v 1.1 2011/03/10 20:02:27 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/zfs

TESTS_SH=	t_zpool

# Manual native stress suite; requires an explicitly supplied scratch pool.
PROGS+=		h_zfs_stress
MAN.h_zfs_stress=	# empty
BINDIR=		${TESTSDIR}
FILESDIR=	${TESTSDIR}
FILES+=		run_native.sh README

.include <bsd.test.mk>
