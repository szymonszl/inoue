#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <curl/curl.h>
#include "json.h"

static struct {
    char *username;
    char *token;
    char *useragent;
	char *filenameformat;
} config = {0};

int
loadcfg()
{
	FILE* f = fopen("inoue.cfg", "r");
	if (!f) {
		perror("inoue: couldnt open config file");
		return 0;
	}
	char key[32];
	char val[512];
	while (fscanf(f, "%32[^ \n] %512[^\n]%*c", key, val) == 2) {
		if (key[0] == '#') continue; // ignore comments
		else if (0 == strcmp(key, "username")) config.username = strdup(val);
		else if (0 == strcmp(key, "token")) config.token = strdup(val);
		else if (0 == strcmp(key, "useragent")) config.useragent = strdup(val);
		else if (0 == strcmp(key, "filenameformat")) config.filenameformat = strdup(val);
		else fprintf(stderr, "Warning: unrecognized key \"%s\" in config\n", key);
	}
	fclose(f);
	if (!config.useragent) {
		config.useragent = "Mozilla/5.0 (only pretending; Inoue/v1)";
	}
	if (!config.filenameformat) {
		config.filenameformat = "%Y-%m-%d_%H-%M_%O.ttr";
	}
	if (!config.username) {
		fprintf(stderr, "No username specified!\n");
		return 0;
	}
	if (!config.token) {
		fprintf(stderr, "No token specified!\n");
		return 0;
	}

	for (int i = 0; config.username[i] != 0; i++)
		config.username[i] = tolower(config.username[i]);

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
	struct tm ts;
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
			// remove milliseconds from the timestamp
			char copy[64];
			char tmp[64] = {0};
			strncpy(copy, ts->string, 64);
			char *part = strtok(copy, ".");
			strcat(tmp, part);
			strcat(tmp, " ");
			part = strtok(NULL, "");
			strcat(tmp, &part[3]);
			memset(&game->ts, 0, sizeof(struct tm));
			if (strptime(ts->string, "%Y-%m-%dT%H:%M:%S %Z", &game->ts)) {
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
								struct json_object_s *u = json_value_as_object(j->value);
								for (struct json_object_element_s *k = u->start; k != NULL; k = k->next) {
									if (0 == strcmp(k->name->string, "username")) {
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
	struct json_parse_result_s result;
	struct json_value_s *root = json_parse_ex(apiresp, apiresplen, 0, NULL, NULL, &result);
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
				count += parse_game(json_value_as_object(j->value), &games[count]);
			}
		}
	}
	free(root);
	return count;
}

int
save_game_to_file(const char *apiresp, size_t apiresplen, FILE *f)
{
	int ret = 1;
	struct json_value_s *root = json_parse(apiresp, apiresplen);
	if (!root)
		return 0;
	struct json_object_s *root_obj = json_value_as_object(root);
	if (!root_obj) {
		free(root);
		return 0;
	}
	for (struct json_object_element_s *i = root_obj->start; i != NULL; i = i->next) {
		if (0 == strcmp(i->name->string, "success")) {
			if (!json_value_is_true(i->value)) {
				free(root);
				return 0;
			}
		} else if (0 == strcmp(i->name->string, "game")) {
			size_t len;
			void *output = json_write_minified(i->value, &len);
			len--; // do not include the NUL
			size_t written = fwrite(output, 1, len, f);
			free(output);
			free(root);
			if (written == len) {
				return 1;
			} else {
				return 0;
			}
		}
	}
}

const char *
generate_filename(game *g)
{
	static buffer *buf = NULL;
	char tmp[32];
	char fmt[] = "%_";
	if (!buf)
		buf = buffer_new();
	if (!buf)
		return NULL;
	buffer_truncate(buf);
	for (int i = 0; config.filenameformat[i] != 0; i++) {
		if (config.filenameformat[i] == '%') {
			i++;
			switch(config.filenameformat[i]) {
			case 'Y':
			case 'y':
			case 'm':
			case 'd':
			case 'H':
			case 'M':
			case 'S':
			case 's':
				fmt[1] = config.filenameformat[i];
				strftime(tmp, 32, fmt, &g->ts);
				buffer_append(buf, tmp, strlen(tmp));
				break;
			case 'o':
			case 'O':
				strncpy(tmp, g->opponent, 32);
				for (int j = 0; tmp[j] != 0; j++) {
					if (config.filenameformat[i] == 'o')
						tmp[j] = tolower(tmp[j]);
					else
						tmp[j] = toupper(tmp[j]);
				}
				buffer_append(buf, tmp, strlen(tmp));
				break;
			case 'u':
				buffer_append(buf, config.username, strlen(config.username));
				break;
			case 'U':
				strncpy(tmp, config.username, 32);
				for (int j = 0; tmp[j] != 0; j++) {
					tmp[j] = toupper(tmp[j]);
				}
				buffer_append(buf, tmp, strlen(tmp));
				break;
			case 'r':
				buffer_append(buf, g->replayid, strlen(g->replayid));
				break;
			case '%':
				buffer_append(buf, "%", 1);
				break;
			default:
				return NULL;
			}
		} else {
			tmp[0] = config.filenameformat[i];
			tmp[1] = 0;
			buffer_append(buf, tmp, 1);
		}
	}
	return buffer_getstr(buf);
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

	if (argc == 2) {
		if (chdir(argv[1]) < 0) {
			perror("inoue: couldn't change directory");
			return EXIT_FAILURE;
		}
	}

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
	// curl_easy_setopt(hnd, CURLOPT_FAILONERROR, 1);

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
	buffer_truncate(buf);

	printf("Resolved UserID: '%s'\n", userid);
	snprintf(url_buf, 128, "https://ch.tetr.io/api/streams/league_userrecent_%s", userid);
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
		char ts[64];
		strftime(ts, 64, "%Y-%m-%d %H:%M", &games[i].ts);
		printf("Found game '%s' against '%s', played on %s\n", games[i].replayid, games[i].opponent, ts);
	}

	struct curl_slist *slist1 = NULL;
	char auth[256];
	snprintf(auth, 256, "Authorization: Bearer %s", config.token);
	slist1 = curl_slist_append(slist1, auth);
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
	for (int i = 0; i < gamec; i++) {
		char *filename = generate_filename(&games[i]);
		if(access(filename, F_OK) != -1 ) {
			printf("Game %s already saved, skipping...\n", games[i].replayid);
			continue;
		}
		FILE *f = fopen(filename, "w");
		if (!f) {
			perror("inoue: couldnt open output file");
			goto main_cleanup;
		}
		snprintf(url_buf, 128, "https://tetr.io/api/games/%s", games[i].replayid);
		curl_easy_setopt(hnd, CURLOPT_URL, url_buf);
		buffer_truncate(buf);
		printf("Downloading %s...\n", games[i].replayid);
		ret = curl_easy_perform(hnd);
		if (ret != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));
			fclose(f);
			unlink(filename); // delete the failed file, so the download can be retried
			goto main_cleanup;
		}
		if (!save_game_to_file(buffer_getstr(buf), buffer_getstrlen(buf), f)) {
			fprintf(stderr, "Saving failed!\n");
			fclose(f);
			unlink(filename);
			continue;
		}
		fclose(f);

	}

	// if the program is exited via goto, then it will return FAILURE instead
	exitcode = EXIT_SUCCESS;
main_cleanup:
	curl_easy_cleanup(hnd);
	buffer_free(buf);

	return exitcode;
}