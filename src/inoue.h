#ifndef _INOUE_H
#define _INOUE_H

#include <string.h>

extern struct _cfg {
	char *username;
	char *apiurl;
	char *useragent;
	char *filenameformat;
} config;
int loadcfg(void);
int loadcfgnew(void);

const char *resolve_username(const char *);

struct json_value_s *json_getpath(struct json_object_s *, const char *);
struct json_object_s *json_get_api_data(struct json_value_s *);
int parse_ts(struct tm *, const char *);
int endswith(const char *, const char *);

typedef struct buffer buffer;
buffer *buffer_new(void);
void buffer_free(buffer *b);
size_t buffer_appendbytes(buffer *b, char *data, size_t length);
size_t buffer_appendstr(buffer *b, char *str);
size_t buffer_appendchar(buffer *b, char c);
const char *buffer_str(buffer *b);
size_t buffer_strlen(buffer *b);
void buffer_truncate(buffer *b);
int buffer_save(buffer *b, FILE *f);
void buffer_load(buffer *b, FILE *f);

int http_init(void);
int http_get(const char *, buffer *, long *);
void http_deinit(void);

#endif