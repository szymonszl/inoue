#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>
#include <string.h>

typedef struct {
	size_t size;
	size_t cursor;
	char *buf;
} buffer;

buffer *buffer_new();
void buffer_free(buffer *b);

size_t buffer_appendbytes(buffer *b, char *data, size_t length);
size_t buffer_appendstr(buffer *b, char *str);
size_t buffer_appendchar(buffer *b, char c);

const char *buffer_str(buffer *b);
size_t buffer_strlen(buffer *b);

void buffer_truncate(buffer *b);

#endif