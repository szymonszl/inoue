#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "inoue.h"

static CURL *hnd = NULL;
static size_t
recv_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return buffer_appendbytes((buffer *)userdata, ptr, size*nmemb);
}

int
http_init(void)
{
	logI("curl ver: %s", curl_version());
	curl_global_init(CURL_GLOBAL_ALL);
	hnd = curl_easy_init();
	if (!hnd) return 0;
	curl_easy_setopt(hnd, CURLOPT_USERAGENT, "Mozilla/5.0 (only pretending; Inoue/v1)");
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, recv_callback);
	// curl_easy_setopt(hnd, CURLOPT_FAILONERROR, 1);
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