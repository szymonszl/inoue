#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "json.h"

#include "inoue.h"

int
loadcfg(void)
{
	FILE* f = fopen("inoue.cfg", "r");
	if (!f) {
		perror("inoue: couldnt open config file");
		return 0;
	}
	char buf[512];
	char key[32];
	char val[256];
	while (fgets(buf, 511, f)) {
		if (sscanf(buf, "%32[^ \n] %256[^\n]%*c", key, val) != EOF) {
			if (key[0] == '#')
				continue; // ignore comments
			else if (0 == strcmp(key, "username"))
				config.username = strdup(val);
			else if (0 == strcmp(key, "apiurl"))
				config.apiurl = strdup(val);
			else if (0 == strcmp(key, "useragent"))
				config.useragent = strdup(val);
			else if (0 == strcmp(key, "filenameformat"))
				config.filenameformat = strdup(val);
			else fprintf(stderr, "Warning: unrecognized key \"%s\" in config\n", key);
		}
	}
	fclose(f);
	if (!config.useragent) {
		config.useragent = "Mozilla/5.0 (only pretending; Inoue/v1)";
	}
	if (!config.filenameformat) {
		config.filenameformat = "%Y-%m-%d_%H-%M_%O.ttrm";
	}
	if (!config.username) {
		fprintf(stderr, "No username specified!\n");
		return 0;
	}
	if (!config.apiurl) {
		config.apiurl = "https://inoue.szy.lol/api/replay/%s";
	}

	for (int i = 0; config.username[i] != 0; i++)
		config.username[i] = tolower(config.username[i]);

	return 1;
}

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