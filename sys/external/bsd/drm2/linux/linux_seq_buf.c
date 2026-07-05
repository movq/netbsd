/* Public domain. */

#include <sys/cdefs.h>
#include <sys/stdarg.h>
#include <sys/systm.h>

#include <linux/seq_buf.h>

int
linux_seq_buf_printf(struct seq_buf *s, const char *fmt, ...)
{
	va_list ap;
	size_t available;
	int len;

	if (s->overflowed || s->pos >= s->size) {
		s->overflowed = true;
		return -1;
	}

	available = s->size - s->pos;
	va_start(ap, fmt);
	len = vsnprintf(s->buf + s->pos, available, fmt, ap);
	va_end(ap);

	if (len < 0 || (size_t)len >= available) {
		s->pos = s->size - 1;
		s->overflowed = true;
		return -1;
	}

	s->pos += len;
	return 0;
}
