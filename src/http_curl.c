#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>

#include "inoue.h"

char *update = NULL; // extern definition

static CURL *hnd = NULL;
static size_t
recv_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return buffer_appendbytes((buffer *)userdata, ptr, size*nmemb);
}

size_t
header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	const static char *name = "X-Inoue-Update:";
	const size_t namelen = strlen(name);
	size_t ret = size * nitems;
	if (!update) {
		if (nitems < namelen)
			return ret;
		for (size_t i = 0; i < namelen; i++) {
			if (tolower(buffer[i]) != tolower(name[i]))
				return ret;
		}
		update = malloc(32);
		size_t cur = 0;
		for (size_t i = namelen; i < nitems; i++) {
			char c = buffer[i];
			if (isspace(c)) {
				if (cur == 0)
					continue; // skip initial whitespace
				else
					break; // but not trailing
			}
			update[cur++] = c;
		}
		update[cur] = '\0';
	}
	return ret;
}

int
http_init(void)
{
	logI("curl ver: %s", curl_version());
	curl_global_init(CURL_GLOBAL_ALL);
	hnd = curl_easy_init();
	if (!hnd) return 0;
	curl_easy_setopt(hnd, CURLOPT_USERAGENT, "Mozilla/5.0 (only pretending; Inoue/" INOUE_VER "; curl)");
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, recv_callback);
	curl_easy_setopt(hnd, CURLOPT_HEADERFUNCTION, header_callback);
	return 1;
}

void
http_deinit(void)
{
	curl_easy_cleanup(hnd);
}

int
http_get(const char *url, buffer *b, long *status)
{
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, b);
	curl_easy_setopt(hnd, CURLOPT_URL, url);
	CURLcode ret = curl_easy_perform(hnd);
	if (ret != CURLE_OK) {
		logE("network request failed: %s", curl_easy_strerror(ret));
		return 0;
	}
	if (status)
		curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, status);
	return 1;
}