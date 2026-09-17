/* SPDX-License-Identifier: BSD-2-Clause */
#include <sys/param.h>
#include <sys/debug.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/zone.h>

int
mutex_enter_interruptible(kmutex_t *lock)
{
	/*
	 * Native adaptive mutex waits cannot be interrupted. This is used
	 * for the SPA namespace lock, where a long pool import must not
	 * prevent another process from responding to a signal.
	 */
	while (!mutex_tryenter(lock)) {
		int error = kpause("zfsmutex", true, 1, NULL);
		if (error == EINTR || error == ERESTART)
			return (error);
	}
	return (0);
}

uint32_t
zone_get_hostid(void *zone)
{
	VERIFY3P(zone, ==, NULL);
	return ((uint32_t)hostid);
}

char *
kmem_vasprintf(const char *fmt, va_list ap)
{
	va_list copy;
	int len;
	char *str;

	va_copy(copy, ap);
	len = vsnprintf(NULL, 0, fmt, copy);
	va_end(copy);
	VERIFY3S(len, >=, 0);
	str = kmem_alloc((size_t)len + 1, KM_SLEEP);
	va_copy(copy, ap);
	VERIFY3S(vsnprintf(str, (size_t)len + 1, fmt, copy), ==, len);
	va_end(copy);
	return (str);
}
