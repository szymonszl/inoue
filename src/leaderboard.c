#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "winunistd.h"

#include "inoue.h"

/* on pagination:
	heavy TODO. i will likely set up some rules on when to follow "latest" pages, so i am preparing to keep stats on
	that. however,  i am still heavily unsure what to do with the other leaderboards, so heavy TODO on that!
*/

struct prisecter {
	double p, s, t;
};

#define MST 9007199254740991 // Number.MAX_SAFE_INTEGER
static const struct prisecter max_pst = {MST, MST, MST};
// i'd assume that the true "max" value should be DBL_MAX, but this is what tetrio uses
// also ideally they'd be integers so i wouldn't have to worry about rounding errors but yknow

static struct prisecter
get_prisecter(struct json_object_s *game)
{
	struct prisecter ret = {};
	ret.p = json_getdouble(game, "p.pri", MST);
	ret.s = json_getdouble(game, "p.sec", MST);
	ret.t = json_getdouble(game, "p.ter", MST);
	return ret;
}

static int
pst_compare(struct prisecter a, struct prisecter b)
{
	if (a.p > b.p) return 1;
	if (a.p < b.p) return -1;
	if (a.s > b.s) return 1;
	if (a.s < b.s) return -1;
	if (a.t > b.t) return 1;
	if (a.t < b.t) return -1;
	return 0;
}

// for paginating, we use the second highest prisecter. the caller keeps a buffer, initialized to {max, max},
// and this function makes them {x, min}, where (x > min)
static void
pst_buffer_add(struct prisecter psts[2], struct prisecter in)
{
	if (pst_compare(in, psts[0]) < 0) { // if new < psts[0], replace
		psts[0] = in;
	}
	if (pst_compare(psts[0], psts[1]) < 0) { // if psts[0] < psts[1], swap
		struct prisecter tmp = psts[1];
		psts[1] = psts[0];
		psts[0] = tmp;
	}
}

struct dlstats {
	int ok, err, gone, exist;
	struct prisecter next;
};

struct dlstats
download_page(const char *format, const char *user, const char *url)
{
	static buffer *b = NULL;
	if (!b) b = buffer_new();
	long status;
	struct dlstats ret = { -1 };
	struct prisecter psts[2] = {max_pst, max_pst};
		if (!http_get(url, b, &status)) {
			return ret;
		}
	if (status != 200) {
		logE("api: received error %ld from server", status);
		return ret;
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
				struct json_object_s *game = json_value_as_object(j->value);
				pst_buffer_add(psts, get_prisecter(game));
				enum dlresult res = download_game(game, format, user);
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
	ret.next = psts[0];
	return ret;
}

void
download_leaderboard(const char *format, const char *user, const char *gamemode, const char *leaderboard)
{
	struct prisecter before = max_pst;
	for (;;) {
		char url[256];
		snprintf(url, 256, "https://ch.tetr.io/api/users/%s/records/%s/%s?limit=100&after=%f:%f:%f",
			user, gamemode, leaderboard, before.p, before.s, before.t);
		logI("url: %s", url);
		struct dlstats ds = download_page(format, user, url);
		if (ds.ok < 0) {
			logE("failed to download leaderboard page!");
			return;
		}
		if (ds.ok + ds.err + ds.exist + ds.gone < 100) {
			logI("no further records exist");
			return;
		}
		if (ds.gone) {
			logI("no further replays available (pruned)");
			return;
		}
		if (ds.err) {
			logE("%d replays failed!\n", ds.err);
		}
		before = ds.next;
	}
}
