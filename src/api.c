#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "json.h"
#include "winunistd.h"

#include "inoue.h"

static char *
_resolve_username(const char *username)
{
	static buffer *b = NULL;
	if (!b) b = buffer_new();
	buffer_truncate(b);
	char url_buf[128];
	snprintf(url_buf, 128, "https://ch.tetr.io/api/users/%s", username);
	if (!http_get(url_buf, b, NULL))
		return NULL;

	char *userid = NULL;
	struct json_value_s *root = json_parse(buffer_str(b), buffer_strlen(b));
	struct json_object_s *data = json_get_api_data(root);
	if (data) {
		struct json_value_s *idv = json_getpath(data, "user._id");
		struct json_string_s *id = json_value_as_string(idv);
		if (id) {
			userid = strdup(id->string);
		}
	}
	free(root);
	return userid;
}

const char *
resolve_username(const char *username)
{
	static struct cache {
		struct cache *next;
		char *username;
		char *userid;
	} *cache = NULL;
	for (struct cache *c = cache; c; c = c->next) {
		if (strcmp(username, c->username) == 0) {
			return c->userid;
		}
	}
	char *uid = _resolve_username(username);
	if (uid) {
		struct cache *c = malloc(sizeof(struct cache));
		c->next = cache;
		c->username = strdup(username);
		c->userid = uid;
		cache = c;
	}
	return uid;
}

void
download_from_stream(const char *url, const char *format, const char *user)
{
	static buffer *b = NULL;
	if (!b) b = buffer_new();
	long status;
	long retries = 0;
	for (;;) {
		buffer_truncate(b);
		if (!http_get(url, b, &status)) {
			return;
		}
		if (status != 429 || retries >= 3) {
			break;
		}
		logI("...throttled, retrying in %ds...", 1<<retries);
		sleep(1 << retries);
		retries++;
	}
	if (status != 200) {
		logE("api: received error %ld from server", status);
	}
	struct json_value_s *root = json_parse(buffer_str(b), buffer_strlen(b));
	if (!root) {
		logE("api: invalid response from server");
		return;
	}
	struct json_object_s *data = json_get_api_data(root);
	if (data) {
		struct json_value_s *rv = json_getpath(data, "records");
		struct json_array_s *records = json_value_as_array(rv);
		if (records) {
			for (struct json_array_element_s *j = records->start; j != NULL; j = j->next) {
				download_game(json_value_as_object(j->value), format, user);
			}
		}
	}
	free(root);
}