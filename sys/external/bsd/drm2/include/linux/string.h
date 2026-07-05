/*	$NetBSD: string.h,v 1.11 2021/12/19 11:45:01 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/null.h>
#include <sys/systm.h>

#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/slab.h>

static inline void *
memchr_inv(const void *buffer, int c, size_t len)
{
	const uint8_t byte = c;	/* XXX lose */
	const char *p;

	for (p = buffer; len-- > 0; p++)
		if (*p != byte)
			return __UNCONST(p);

	return NULL;
}

static inline bool
mem_is_zero(const void *b, size_t len)
{
	return (memchr_inv(b, 0, len) == NULL);
}

static inline void *
kmemdup(const void *src, size_t len, gfp_t gfp)
{
	void *dst;

	dst = kmalloc(len, gfp);
	if (dst == NULL)
		return NULL;

	(void)memcpy(dst, src, len);
	return dst;
}

static inline void *
memdup_user(const void __user *src, size_t len)
{
	void *dst;

	dst = kmalloc(len, GFP_KERNEL);
	if (dst == NULL)
		return ERR_PTR(-ENOMEM);

	if (copyin(src, dst, len) != 0) {
		kfree(dst);
		return ERR_PTR(-EFAULT);
	}

	return dst;
}

static inline void *
memdup_array_user(const void __user *src, size_t n, size_t size)
{

	if (n != 0 && size > SIZE_MAX/n)
		return ERR_PTR(-EOVERFLOW);

	return memdup_user(src, n * size);
}

static inline void *
vmemdup_array_user(const void __user *src, size_t n, size_t size)
{

	/*
	 * NetBSD's kvmalloc and kmalloc compatibility implementations use
	 * the same allocator, and kvfree accepts allocations from it.
	 */
	return memdup_array_user(src, n, size);
}

static inline void *
memdup_user_nul(const void __user *src, size_t len)
{
	char *dst;

	if (len == SIZE_MAX)
		return ERR_PTR(-ENOMEM);

	dst = kmalloc(len + 1, GFP_KERNEL);
	if (dst == NULL)
		return ERR_PTR(-ENOMEM);

	if (copyin(src, dst, len) != 0) {
		kfree(dst);
		return ERR_PTR(-EFAULT);
	}
	dst[len] = '\0';

	return dst;
}

static inline char *
kstrndup(const char *src, size_t maxlen, gfp_t gfp)
{
	char *dst;
	size_t len;

	if (src == NULL)
		return NULL;

	len = strnlen(src, maxlen);
	dst = kmalloc(len + 1, gfp);
	if (dst == NULL)
		return NULL;

	(void)memcpy(dst, src, len);
	dst[len] = '\0';

	return dst;
}

static inline char *
kstrdup(const char *src, gfp_t gfp)
{

	if (src == NULL)
		return NULL;
	return kstrndup(src, strlen(src), gfp);
}

static inline const char *
kstrdup_const(const char *src, gfp_t gfp)
{

	return kstrdup(src, gfp);
}

static inline void
kfree_const(const void *ptr)
{

	kfree(__UNCONST(ptr));
}

static inline ssize_t
linux_sized_strscpy(char *dst, const char *src, size_t dstsize)
{
	size_t n = dstsize;

	/* If no space for a NUL terminator, fail.  */
	if (n == 0)
		return -E2BIG;

	/* Copy until we get a NUL terminator or the end of the buffer.  */
	while ((*dst++ = *src++) != '\0') {
		if (__predict_false(--n == 0)) {
			dst[-1] = '\0'; /* NUL-terminate */
			return -E2BIG;
		}
	}

	/* Return the number of bytes copied, excluding NUL.  */
	return dstsize - n;
}

#define	__linux_strscpy2(dst, src)	\
	linux_sized_strscpy((dst), (src), sizeof(dst))
#define	__linux_strscpy3(dst, src, size)	\
	linux_sized_strscpy((dst), (src), (size))
#define	__linux_strscpy_pick(_1, _2, _3, fn, ...)	fn
#define	strscpy(...)							\
	__linux_strscpy_pick(__VA_ARGS__, __linux_strscpy3,		\
	    __linux_strscpy2)(__VA_ARGS__)

static inline ssize_t
strscpy_pad(char *dst, const char *src, size_t dstsize)
{
	ssize_t wrote;

	wrote = strscpy(dst, src, dstsize);
	if (wrote >= 0 && (size_t)wrote < dstsize)
		memset(dst + wrote + 1, 0, dstsize - wrote - 1);
	return wrote;
}

static inline char *
strnchr(const char *s, size_t count, int c)
{

	while (count--) {
		if (*s == (char)c)
			return __UNCONST(s);
		if (*s++ == '\0')
			break;
	}
	return NULL;
}

static inline char *
strnstr(const char *s, const char *find, size_t len)
{
	size_t findlen;

	findlen = strlen(find);
	if (findlen == 0)
		return __UNCONST(s);
	while (len >= findlen) {
		if (memcmp(s, find, findlen) == 0)
			return __UNCONST(s);
		s++;
		len--;
	}
	return NULL;
}

static inline void *
memset32(uint32_t *buf, uint32_t v, size_t n)
{
	uint32_t *p = buf;

	while (n --> 0)
		*p++ = v;

	return buf;
}

static inline void *
memset64(uint64_t *buf, uint64_t v, size_t n)
{
	uint64_t *p = buf;

	while (n --> 0)
		*p++ = v;

	return buf;
}

static inline void *
memset_p(void **buf, void *v, size_t n)
{
	void **p = buf;

	while (n --> 0)
		*p++ = v;

	return buf;
}

static inline size_t
str_has_prefix(const char *str, const char *prefix)
{
	size_t len = strlen(prefix);

	return strncmp(str, prefix, len) == 0 ? len : 0;
}

static inline int
match_string(const char *const *haystack, size_t n, const char *needle)
{
	int i;

	for (i = 0; i < n; i++) {
		if (haystack[i] == NULL)
			break;
		if (strcmp(haystack[i], needle) == 0)
			return i;
		if (i == INT_MAX)
			break;
	}
	return -EINVAL;
}

#endif  /* _LINUX_STRING_H_ */
