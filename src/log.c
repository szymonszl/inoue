#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include "inoue.h"

enum log_level log_maxseen = LOG_INFO; // extern definition

void
log_(enum log_level l, const char *fmt, ...)
{
	if (l > log_maxseen)
		log_maxseen = l;
	fprintf(stderr, "[%c] ", " WE"[l]);
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

void
logS(const char *fmt, ...)
{
	int e = errno;
	fprintf(stderr, "[E] ");
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, ": %s\n", strerror(e));
}