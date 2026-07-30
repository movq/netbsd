#	$NetBSD: Makefile,v 1.7 2016/01/23 21:22:46 christos Exp $

.include <bsd.obj.mk>		# Pull in OBJDIR name rules.
.include <bsd.own.mk>
.include <bsd.klinks.mk>

LIBISPRIVATE?=	yes
LIB=	kern
DBG=	-Os

S= ${NETBSDSRCDIR}/sys
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S} -I${S}/arch

CPPFLAGS+= -D_STANDALONE

COPTS+=	-ffreestanding
COPTS+=	-fno-stack-protector
COPTS+=	-fno-unwind-tables
CWARNFLAGS+= -Werror
CWARNFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CWARNFLAGS+= ${${ACTIVE_CC} == "clang":? \
	-Wno-error=alloc-size \
	-Wno-error=array-compare \
	-Wno-error=bitwise-instead-of-logical \
	-Wno-error=cast-function-type-mismatch \
	-Wno-error=default-const-init-field-unsafe \
	-Wno-error=deprecated-non-prototype \
	-Wno-error=format-truncation \
	-Wno-error=fortify-source \
	-Wno-error=missing-format-attribute \
	-Wno-error=null-pointer-subtraction \
	-Wno-error=sometimes-uninitialized \
	-Wno-error=single-bit-bitfield-constant-conversion \
	-Wno-error=strict-prototypes \
	-Wno-error=tautological-compare \
	-Wno-error=unterminated-string-initialization \
	-Wno-error=uninitialized-const-pointer \
	-Wno-error=unused-but-set-parameter \
	-Wno-error=unused-but-set-variable \
	-Wno-error=vla-cxx-extension :}
CPPFLAGS+= -D_KERNTYPES

### find out what to use for libkern
KERN_AS=	library
KERNDIR=	${S}/lib/libkern

.include "${KERNDIR}/Makefile.libkern"
.ifndef ARCHSUBDIR
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

.include <bsd.lib.mk>
