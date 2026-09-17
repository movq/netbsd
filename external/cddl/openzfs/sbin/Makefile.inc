# $NetBSD$
.include "../Makefile.inc"

BINDIR=	/sbin
MAN=	# Each command lists its manual pages.

.PATH:	${ZFSDIR}/man/man7 ${ZFSDIR}/man/man8

# Resolve only libraries built from this import, never the installed osnet
# libraries. The libraries have mutual references (as upstream's shared
# libraries do), so use an archive group for this private build.
LDADD+=	-Wl,--start-group
.for zfslib in zfs zfs_core zutil nvpair uutil avl tpool share spl
ZFS_LIBDIR.${zfslib}!= ${MAKE} -C ${ZFSNETBSD}/lib/lib${zfslib} -V .OBJDIR
DPADD+=	${ZFS_LIBDIR.${zfslib}}/lib${zfslib}.a
LDADD+=	${ZFS_LIBDIR.${zfslib}}/lib${zfslib}.a
.endfor
LDADD+=	-Wl,--end-group -lcrypto -lz -lm -lutil -lpthread -lintl
