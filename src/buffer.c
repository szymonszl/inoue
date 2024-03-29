#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inoue.h"

struct buffer {
	size_t size;
	size_t cursor;
	char *buf;
};

buffer *
buffer_new(void)
{
	buffer *b = malloc(sizeof(buffer));
	if (!b)
		return NULL;
	b->buf = malloc(8192);
	if (!b->buf) {
		free(b);
		return NULL;
	}
	b->size = 8192;
	b->cursor = 0;
	return b;
}

int
ensure_size(buffer *b, size_t length) {
	if (b->cursor + length + 1 > b->size) {
		size_t newlen = ((b->cursor + length + 8193) / 8192 ) * 8192;
		char *newbuf = realloc(b->buf, newlen);
		if (!newbuf) {
			return 0;
		}
		b->buf = newbuf;
	}
	return 1;
}

size_t
buffer_appendbytes(buffer *b, const char *data, size_t length)
{
	if (!ensure_size(b, length))
		return 0;
	memcpy(&b->buf[b->cursor], data, length);
	b->cursor += length;
	b->buf[b->cursor] = 0;
	/* cursor is not incremented, because the next call should overwrite the null */
	return length;
}

size_t
buffer_appendstr(buffer *b, const char *str)
{
	return buffer_appendbytes(b, str, strlen(str));
}

size_t
buffer_appendchar(buffer *b, char c)
{
	if (!ensure_size(b, 1))
		return 0;
	b->buf[b->cursor++] = c;
	b->buf[b->cursor] = 0;
	return 1;
}

const char*
buffer_str(buffer *b)
{
	if (!b)
		return NULL;
	return b->buf;
}

size_t
buffer_strlen(buffer *b)
{
	if (!b)
		return -1;
	return b->cursor;
}

void
buffer_truncate(buffer *b)
{
	b->cursor = 0;
	b->buf[0] = 0;
}

void
buffer_free(buffer *b)
{
	if (!b)
		return;
	if (b->buf)
		free(b->buf);
	b->buf = 0;
	free(b);
}

int
buffer_save(buffer *b, FILE *f)
{
	size_t written = fwrite(buffer_str(b), 1, buffer_strlen(b), f);
	if (written == buffer_strlen(b)) {
		return 1;
	}
	logS("failed to save");
	return 0;
}

void
buffer_load(buffer *b, FILE *f)
{
	char buf[1024];
	int rl;
	while ((rl = fread(buf, 1, 1024, f))) {
		buffer_appendbytes(b, buf, rl);
	}
	return;
}
