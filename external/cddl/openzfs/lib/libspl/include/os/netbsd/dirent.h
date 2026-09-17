/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _LIBSPL_NETBSD_DIRENT_H_
#define	_LIBSPL_NETBSD_DIRENT_H_
#include_next <dirent.h>
#define	dirent64	dirent
#define	readdir64	readdir
#endif
