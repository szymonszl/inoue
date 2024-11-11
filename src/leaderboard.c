#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "json.h"
#include "winunistd.h"

#include "inoue.h"

/* on pagination:
	heavy TODO. i will likely set up some rules on when to follow "latest" pages, so i am preparing to keep stats on
	that. however,  i am still heavily unsure what to do with the other leaderboards, so heavy TODO on that!
*/

struct dlstats {
	int ok, err, gone, exist;
};

struct dlstats
download_page(const char *format, const char *user, const char *url)
{
	static buffer *b = NULL;
	if (!b) b = buffer_new();
	long status;
	long retries = 0;
	struct dlstats ret = { -1 };
	for (;;) {
		buffer_truncate(b);
		if (!http_get(url, b, &status)) {
			return ret;
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
		return ret;
	}
	struct json_object_s *data = json_get_api_data(root);
	if (data) {
		struct json_value_s *rv = json_getpath(data, "entries");
		struct json_array_s *records = json_value_as_array(rv);
		if (records) {
			ret = (struct dlstats){};
			for (struct json_array_element_s *j = records->start; j != NULL; j = j->next) {
				enum dlresult res = download_game(json_value_as_object(j->value), format, user);
				switch (res) {
				case DL_OK: ret.ok++; break;
				case DL_ERR: ret.err++; break;
				case DL_GONE: ret.gone++; break;
				case DL_EXISTS: ret.exist++; break;
				}
			}
		}
	}
	free(root);
	return ret;
}

void
download_leaderboard(const char *format, const char *user, const char *gamemode, const char *leaderboard)
{
	char url[256];
	snprintf(url, 256, "https://ch.tetr.io/api/users/%s/records/%s/%s?limit=50", user, gamemode, leaderboard);
	// very heavy TODO: pagination! will require dealing with prisecters...
	struct dlstats ds = download_page(format, user, url);
	if (ds.ok < 0) {
		logE("failed to download leaderboard page!");
	}
	if (ds.err) {
		logE("%d replays failed!\n", ds.err);
	}
}
