#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include "winunistd.h"
#include "json.h"

#include "inoue.h"

struct json_value_s *
json_getpath(struct json_object_s *json, const char *path)
{
	struct json_value_s *cur;
	struct json_object_s *obj = json;
	char *pth = strdup(path);
	char *r;
	char *part = strtok_r(pth, ".", &r);
	while (1) {
		cur = NULL;
		for (struct json_object_element_s *i = obj->start; i != NULL; i = i->next) {
			if (0 == strcmp(i->name->string, part)) {
				cur = i->value;
				break;
			}
		}
		if (!cur)
			break;
		part = strtok_r(NULL, ".", &r);
		if (part) {
			obj = json_value_as_object(cur);
			if (!obj) {
				break;
			}
		} else {
			break;
		}
	}
	free(pth);
	return cur;
}

const char *
json_getstring(struct json_object_s *json, const char *path, int empty)
{
	struct json_value_s *v = json_getpath(json, path);
	if (v) {
		struct json_string_s *s = json_value_as_string(v);
		if (s) {
			return s->string;
		}
	}
	if (empty) {
		return "";
	} else {
		return NULL;
	}
}

double
json_getdouble(struct json_object_s *json, const char *path, double fallback)
{
	if (!json) return fallback;
	struct json_value_s *v = json_getpath(json, path);
	if (!v) return fallback;
	struct json_number_s *n = json_value_as_number(v);
	if (!n) return fallback;
	double r;
	if (sscanf(n->number, "%lf", &r) == 1)
		return r;
	return fallback;
}

struct json_object_s *
json_get_api_data(struct json_value_s *root)
{
	struct json_object_s *root_obj = json_value_as_object(root);
	if (!root_obj)
		return NULL;
	for (struct json_object_element_s *i = root_obj->start; i != NULL; i = i->next) {
		if (0 == strcmp(i->name->string, "success")) {
			if (!json_value_is_true(i->value)) {
				logE("error from server: %s", json_getstring(root_obj, "error", 1));
				return NULL;
			}
		} else if (0 == strcmp(i->name->string, "data")) {
			return json_value_as_object(i->value);
		}
	}
	return NULL;
}

int
parse_ts(struct tm *t, const char *str)
{
	// %Y-%m-%dT%H:%M:%S %z
	// 2022-10-23T20:53:14.779Z
	int msec;
	if (sscanf(str, "%u-%u-%uT%u:%u:%u.%uZ",
			&t->tm_year, &t->tm_mon, &t->tm_mday,
			&t->tm_hour, &t->tm_min, &t->tm_sec, &msec) == 7) {
		t->tm_year -= 1900;
		t->tm_mon -= 1;
		return 1;
	}
	return 0;
}

int
endswith(const char *str, const char *suf)
{
	return strcmp(str+strlen(str)-strlen(suf), suf) == 0;
}

int
ensure_dir(const char *path)
{ // note: creates a directory FOR path not AT path, ie creates "a" for "a/b"
	char *dir = strdup(path);
	char *slash = strrchr(dir, '/');
	if (!slash) { // no dirs to create
		free(dir);
		return 1;
	}
	*slash = '\0';
	int r = mkdir(dir, 0777);
	if (r == 0) {
		free(dir);
		return 1;
	}
	if (errno == EEXIST) {
		free(dir);
		return 1;
	}
	if (errno == ENOENT) {
		r = ensure_dir(dir); // try lower dir
		if (r)
			r = ensure_dir(path); // retry
		free(dir);
		return r;
	}
	logS("failed to create directory for %s", path);
	free(dir);
	return 0;
}

char *
getcwd_(void)
{
	int buflen = 256;
	char *buf = NULL;
	while (!buf) {
		buf = malloc(buflen);
		if (getcwd(buf, buflen) == NULL) {
			if (errno == ENOMEM) {
				free(buf);
				buf = NULL;
				buflen <<= 1;
			} else {
				logS("failed to get working directory");
			}
		}
	}
	return buf;
}