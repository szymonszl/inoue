#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "json.h"

static struct {
    char *username;
    char *token;
    char *useragent;
} config = {0};

int
loadcfg()
{
	FILE* f = fopen("inoue.cfg", "r");
	char key[32];
	char val[512];
	while (fscanf(f, "%32[^ \n] %512[^\n]%*c", key, val) == 2) {
		if (key[0] == '#') continue; // ignore comments
		else if (0 == strcmp(key, "username")) config.username = strdup(val);
		else if (0 == strcmp(key, "token")) config.token = strdup(val);
		else if (0 == strcmp(key, "useragent")) config.useragent = strdup(val);
		else fprintf(stderr, "Warning: unrecognized key \"%s\" in config\n", key);
	}
	fclose(f);
	if (!config.useragent) {
		config.useragent = "Mozilla/5.0 (only pretending; Inoue/v1)";
	}
	if (!config.username) {
		fprintf(stderr, "No username specified!\n");
		return 0;
	}
	if (!config.token) {
		fprintf(stderr, "No token specified!\n");
		return 0;
	}
	return 1;
}

typedef struct {
	size_t size;
	size_t cursor;
	char *buf;
} buffer;

buffer *
buffer_new()
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
size_t
buffer_append(buffer *b, char *data, size_t length)
{
	if (b->cursor + length + 1> b->size) {
		size_t newlen = ((b->cursor + length + 8193) / 8192 ) * 8192;
		char *newbuf = realloc(b->buf, newlen);
		if (!newbuf) {
			return 0;
		}
		b->buf = newbuf;
	}
	memcpy(&b->buf[b->cursor], data, length);
	b->cursor += length;
	b->buf[b->cursor] = 0; // cursor is not incremented, because the next call should overwrite the null
	return length;
}

const char*
buffer_getstr(buffer *b)
{
	if (!b)
		return NULL;
	return b->buf;
}

size_t
buffer_getstrlen(buffer *b)
{
	if (!b)
		return -1;
	return b->cursor;
}

void
buffer_truncate(buffer *b)
{
	b->cursor = 0;
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
parse_userid(const char *apiresp, size_t apiresplen, char *userid)
{
	int ret = 1;
	struct json_value_s *root = json_parse(apiresp, apiresplen);
	struct json_object_s *data = json_get_api_data(root);
	if (data) {
		for (struct json_object_element_s *i = data->start; i != NULL; i = i->next) {
			if (0 == strcmp(i->name->string, "user")) {
				for (struct json_object_element_s *j = json_value_as_object(i->value)->start; j != NULL; j = j->next) {
					if (0 == strcmp(j->name->string, "_id")) {
						struct json_string_s *id = json_value_as_string(j->value);
						if (id) {
							ret = 0;
							strcpy(userid, id->string);
							break;
						}
					}
				}
			}
		}
	}
	free(root);
	return ret;
}

typedef struct {
	char opponent[32];
	char replayid[32];
	char ts[32];
} game;

int
parse_game(struct json_object_s *json, game *game)
{
	if (!json)
		return 0;
	int values = 0;
	for (struct json_object_element_s *i = json->start; i != NULL; i = i->next) {
		if (0 == strcmp(i->name->string, "replayid")) {
			struct json_string_s *replayid = json_value_as_string(i->value);
			if (replayid) {
				strcpy(game->replayid, replayid->string);
				values++;
			}
		} else if (0 == strcmp(i->name->string, "ts")) {
			struct json_string_s *ts = json_value_as_string(i->value);
			if (ts) {
				strcpy(game->ts, ts->string);
				values++;
			}
		} else if (0 == strcmp(i->name->string, "endcontext")) {
			struct json_array_s *endcontext = json_value_as_array(i->value);
			if (endcontext) {
				for (struct json_array_element_s *i = endcontext->start; i != NULL; i = i->next) {
					struct json_object_s *c = json_value_as_object(i->value);
					if (c) {
						for (struct json_object_element_s *j = c->start; j != NULL; j = j->next) {
							if (0 == strcmp(j->name->string, "user")) {
								struct json_object_s *u = json_value_as_object(i->value);
								for (struct json_object_element_s *k = u->start; k != NULL; k = k->next) {
									if (0 == strcmp(k->name->string, "user")) {
										struct json_string_s *username = json_value_as_string(k->value);
										if (username) {
											if (0 != strcmp(username->string, config.username)) {
												strcpy(game->opponent, username->string);
												values++;
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return (values==3)?1:0;
}

int
parse_game_list(const char *apiresp, size_t apiresplen, game *games)
{
	printf("AR: %s\n", apiresp);
	struct json_value_s *root = json_parse(apiresp, apiresplen);
	if (!root)
		return 0;
	struct json_object_s *data = json_get_api_data(root);
	int count = 0;
	if (!data) {
		free(root);
		return 0;
	}
	for (struct json_object_element_s *i = data->start; i != NULL; i = i->next) {
		if (0 == strcmp(i->name->string, "records")) {
			struct json_array_s *records = json_value_as_array(i->value);
			if (!records)
				break;
			for (struct json_array_element_s *j = records->start; j != NULL; j = j->next) {
				count += parse_game(json_value_as_object(i->value), &games[count]);
			}
		}
	}
	free(root);
	return count;
}

size_t
recv_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return buffer_append((buffer *)userdata, ptr, size*nmemb);
}

int
main(int argc, char **argv)
{
	printf("INOUE v0.1\n");
	printf("curl ver: %s\n", curl_version());
	int exitcode = EXIT_FAILURE;

	if (!loadcfg()) {
		fprintf(stderr, "Configuration error, exiting!\n");
		return EXIT_FAILURE;
	}

	CURLcode ret;
	CURL *hnd;
	buffer *buf = buffer_new();

	curl_global_init(CURL_GLOBAL_ALL);
	hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_USERAGENT, config.useragent);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, recv_callback);
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, buf);

	puts("Resolving username...");
	char url_buf[128];
	snprintf(url_buf, 128, "https://ch.tetr.io/api/users/%s", config.username);
	curl_easy_setopt(hnd, CURLOPT_URL, url_buf);
	ret = curl_easy_perform(hnd);

	if (ret != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));
		goto main_cleanup;
	}

	char userid[64];
	if (parse_userid(buffer_getstr(buf), buffer_getstrlen(buf), userid)) {
		fprintf(stderr, "Invalid server response while resolving username!\n");
		goto main_cleanup;
	}

	printf("Resolved UserID: '%s'\n", userid);
	buffer_truncate(buf);
	snprintf(url_buf, 128, "http://ch.tetr.io/api/streams/league_userrecent_%s", config.username);
	curl_easy_setopt(hnd, CURLOPT_URL, url_buf);
	ret = curl_easy_perform(hnd);
	if (ret != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));
		goto main_cleanup;
	}

	game games[10];
	int gamec = parse_game_list(buffer_getstr(buf), buffer_getstrlen(buf), games); // FIXME: fails
	if (!gamec) {
		fprintf(stderr, "Error while parsing games, exiting!\n");
		goto main_cleanup;
	}
	for (int i = 0; i < gamec; i++) {
		printf("Found game '%s' against '%s', played on '%s'\n", games[i].replayid, games[i].opponent, games[i].ts);
	}

	// if the program is exited via goto, then it will return FAILURE instead
	exitcode = EXIT_SUCCESS;
main_cleanup:
	curl_easy_cleanup(hnd);
	buffer_free(buf);

	return exitcode;
}