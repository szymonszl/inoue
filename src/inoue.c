#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <curl/curl.h>
#include "json.h"
#include "buffer.h"

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
	char buf[512];
	char key[32];
	char val[256];
	while (fgets(buf, 511, f)) {
		if (sscanf(buf, "%32[^ \n] %256[^\n]%*c", key, val) != EOF) {
			if (key[0] == '#')
				continue; // ignore comments
			else if (0 == strcmp(key, "username"))
				config.username = strdup(val);
			else if (0 == strcmp(key, "token"))
				config.token = strdup(val);
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
		struct json_value_s *idv = json_getpath(data, "user._id");
		struct json_string_s *id = json_value_as_string(idv);
		if (id) {
			ret = 0;
			strcpy(userid, id->string);
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

	struct json_value_s *v = json_getpath(json, "replayid");
	if (!v)
		return 0;
	struct json_string_s *replayid = json_value_as_string(v);
	if (!replayid)
		return 0;
	strcpy(game->replayid, replayid->string);

	v = json_getpath(json, "ts");
	if (!v)
		return 0;
	struct json_string_s *ts = json_value_as_string(v);
	if (!ts)
		return 0;
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
	if (!strptime(tmp, "%Y-%m-%dT%H:%M:%S %z", &game->ts)) {
		return 0;
	}

	v = json_getpath(json, "endcontext");
	struct json_array_s *endcontext = json_value_as_array(v);
	if (endcontext) {
		for (struct json_array_element_s *i = endcontext->start; i != NULL; i = i->next) {
			struct json_object_s *c = json_value_as_object(i->value);
			if (!c)
				return 0;
			v = json_getpath(c, "user.username");
			if (!v)
				return 0;
			struct json_string_s *username = json_value_as_string(v);
			if (!username)
				return 0;
			if (0 != strcmp(username->string, config.username)) {
				strcpy(game->opponent, username->string);
				break;
			}
		}
	}
	return 1;
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
	struct json_value_s *rv = json_getpath(data, "records");
	struct json_array_s *records = json_value_as_array(rv);
	if (!records)
		return count;
	for (struct json_array_element_s *j = records->start; j != NULL; j = j->next) {
		count += parse_game(json_value_as_object(j->value), &games[count]);
	}
	free(root);
	return count;
}

int
save_game_to_file(const char *apiresp, size_t apiresplen, FILE *f)
{
	struct json_value_s *root = json_parse(apiresp, apiresplen);
	if (!root)
		return 0;
	struct json_object_s *root_obj = json_value_as_object(root);
	if (!root_obj) {
		free(root);
		return 0;
	}
	struct json_value_s *v = json_getpath(root_obj, "success");
	if (!json_value_is_true(v)) {
		free(root);
		return 0;
	}

	v = json_getpath(root_obj, "game");
	if (v) {
		size_t len;
		void *output = json_write_minified(v, &len);
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
	free(root);
	return 0;
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
				buffer_appendstr(buf, tmp);
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
				buffer_appendstr(buf, tmp);
				break;
			case 'u':
				buffer_appendstr(buf, config.username);
				break;
			case 'U':
				strncpy(tmp, config.username, 32);
				for (int j = 0; tmp[j] != 0; j++) {
					tmp[j] = toupper(tmp[j]);
				}
				buffer_appendstr(buf, tmp);
				break;
			case 'r':
				buffer_appendstr(buf, g->replayid);
				break;
			case '%':
				buffer_appendchar(buf, '%');
				break;
			default:
				return NULL;
			}
		} else {
			buffer_appendchar(buf, config.filenameformat[i]);
		}
	}
	return buffer_str(buf);
}

size_t
recv_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return buffer_appendbytes((buffer *)userdata, ptr, size*nmemb);
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
	if (parse_userid(buffer_str(buf), buffer_strlen(buf), userid)) {
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
	int gamec = parse_game_list(buffer_str(buf), buffer_strlen(buf), games); // FIXME: fails
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
		const char *filename = generate_filename(&games[i]);
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
		if (!save_game_to_file(buffer_str(buf), buffer_strlen(buf), f)) {
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
