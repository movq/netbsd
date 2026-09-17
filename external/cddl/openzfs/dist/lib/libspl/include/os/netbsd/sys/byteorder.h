/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _LIBSPL_NETBSD_BYTEORDER_H_
#define	_LIBSPL_NETBSD_BYTEORDER_H_
#include <sys/endian.h>
#include <sys/isa_defs.h>
#include <inttypes.h>

#define	BSWAP_8(x)	((x) & 0xff)
#define	BSWAP_16(x)	bswap16(x)
#define	BSWAP_32(x)	bswap32(x)
#define	BSWAP_64(x)	bswap64(x)
#define	BE_8(x)		BSWAP_8(x)
#define	LE_8(x)		BSWAP_8(x)
#define	BE_16(x)	htobe16(x)
#define	BE_32(x)	htobe32(x)
#define	BE_64(x)	htobe64(x)
#define	LE_16(x)	htole16(x)
#define	LE_32(x)	htole32(x)
#define	LE_64(x)	htole64(x)
#define	BE_IN8(p)	(*(const uint8_t *)(p))
#define	BE_IN16(p)	be16dec(p)
#define	BE_IN32(p)	be32dec(p)
#define	BE_IN64(p)	be64dec(p)
#define	LE_IN8(p)	(*(const uint8_t *)(p))
#define	LE_IN16(p)	le16dec(p)
#define	LE_IN32(p)	le32dec(p)
#define	LE_IN64(p)	le64dec(p)
#define	BE_OUT8(p, v)	(*(uint8_t *)(p) = (v))
#define	BE_OUT16(p, v)	be16enc(p, v)
#define	BE_OUT32(p, v)	be32enc(p, v)
#define	BE_OUT64(p, v)	be64enc(p, v)
#define	LE_OUT8(p, v)	(*(uint8_t *)(p) = (v))
#define	LE_OUT16(p, v)	le16enc(p, v)
#define	LE_OUT32(p, v)	le32enc(p, v)
#define	LE_OUT64(p, v)	le64enc(p, v)
#define	htonll(x)	htobe64(x)
#define	ntohll(x)	be64toh(x)
#endif
