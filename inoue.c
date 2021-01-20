#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

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

	if (!loadcfg()) {
		fprintf(stderr, "Configuration error, exiting!\n");
		return EXIT_FAILURE;
	}

	CURLcode ret;
	CURL *hnd;
	buffer *buf = buffer_new();

	curl_global_init(CURL_GLOBAL_ALL);
	hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_URL, "https://ch.tetr.io/api/streams/league_userrecent_5e332c0b9380f13edda2b1b8");
	curl_easy_setopt(hnd, CURLOPT_USERAGENT, config.useragent);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, recv_callback);
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, buf);
	puts("Executing GET...");
	ret = curl_easy_perform(hnd);

	if (ret != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(ret));
	} else {
		puts("Downloaded data from tetra channel:");
		puts(buf->buf);
		printf("\n");
	}

	curl_easy_cleanup(hnd);
	hnd = NULL;

	return 0;
}