#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "json.h"

#include "inoue.h"

void
download_game(struct json_object_s *game, const char *format, const char *apiurl)
{
	if (!game)
		return;

	const char *replayid = json_getstring(game, "replayid", 0);
	if (!replayid)
		return;

	const char *rawts = json_getstring(game, "ts", 0);
	if (!rawts)
		return;
	struct tm ts;
	memset(&ts, 0, sizeof(struct tm));
	if (!parse_ts(&ts, rawts)) {
		return;
	}

	printf("rid: %s\n", replayid);
	return;
}
