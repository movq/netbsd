# Makefile for tbl

PROG=		tbl
SRCS=		main.cc table.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

.include <bsd.prog.mk>
.include "../../Makefile.inc"
.include "../Makefile.cfg"
