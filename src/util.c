#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
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
	const char *ret = NULL;
	if (!json) goto out;
	struct json_value_s *v = json_getpath(json, path);
	if (!v) goto out;
	struct json_string_s *s = json_value_as_string(v);
	if (!s) goto out;
	ret = s->string;
out:
	if (ret)
		return ret;
	if (empty) {
		return "";
	} else {
		return NULL;
	}
}

struct json_object_s *
json_get_api_data(struct json_value_s *root)
{
	struct json_object_s *root_obj = json_value_as_object(root);
	if (!root_obj)
		return NULL;
	for (struct json_object_element_s *i = root_obj->start; i != NULL; i = i->next) {
		if (0 == strcmp(i->name->string, "success")) {
			if (!json_value_is_true(i->value))
				return NULL;
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