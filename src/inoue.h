#ifndef _INOUE_H
#define _INOUE_H

#include <string.h>
#include "json.h"

extern struct _cfg {
	char *username;
	char *apiurl;
	char *useragent;
	char *filenameformat;
} config;
int loadcfg(void);

struct json_value_s *json_getpath(struct json_object_s *, const char *);

typedef struct buffer buffer;
buffer *buffer_new(void);
void buffer_free(buffer *b);
size_t buffer_appendbytes(buffer *b, char *data, size_t length);
size_t buffer_appendstr(buffer *b, char *str);
size_t buffer_appendchar(buffer *b, char c);
const char *buffer_str(buffer *b);
size_t buffer_strlen(buffer *b);
void buffer_truncate(buffer *b);


#endif