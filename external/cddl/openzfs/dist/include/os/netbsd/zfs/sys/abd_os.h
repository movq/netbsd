/* SPDX-License-Identifier: CDDL-1.0 */
/*
 * Copyright (c) 2014 by Chunwei Chen. All rights reserved.
 * Copyright (c) 2016, 2019 by Delphix. All rights reserved.
 */
#ifndef _NETBSD_ABD_OS_H_
#define	_NETBSD_ABD_OS_H_

/* Like the FreeBSD port, scatter ABDs use an array of mapped chunks. */
struct abd_scatter {
	uint_t	abd_offset;
	void	*abd_chunks[1];	/* variable length */
};

struct abd_linear {
	void	*abd_buf;
};

#endif
