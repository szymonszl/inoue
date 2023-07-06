#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "winunistd.h"
#include "json.h"

#include "inoue.h"

static const char *
get_opponent(struct json_object_s *game, const char *user)
{
	struct json_value_s *v = json_getpath(game, "endcontext");
	struct json_array_s *endcontext = json_value_as_array(v);
	if (endcontext) {
		for (struct json_array_element_s *i = endcontext->start; i != NULL; i = i->next) {
			struct json_object_s *c = json_value_as_object(i->value);
			if (!c)
				return "";
			const char *username = json_getstring(c, "user.username", 0);
			if (!username) continue;
			if (0 != strcmp(username, user)) {
				return username;
			}
		}
	}
	return "";
}

static const char *
generate_filename(struct json_object_s *game, const char *format, const char *replayid, const char *user)
{
	static buffer *buf = NULL;
	char tmp[32];
	char fmt[] = "%_";
	if (!buf)
		buf = buffer_new();
	buffer_truncate(buf);
	struct tm ts;
	memset(&ts, 0, sizeof(struct tm));
	const char *rawts = json_getstring(game, "ts", 0);
	if (rawts) {
		if (!parse_ts(&ts, rawts)) {
			logE("failed to parse game timestamp: %s", rawts);
			return NULL;
		}
	}
	for (int i = 0; format[i] != 0; i++) {
		if (format[i] == '%') {
			i++;
			switch(format[i]) {
			case 'Y':
			case 'y':
			case 'm':
			case 'd':
			case 'H':
			case 'M':
			case 'S':
			case 's':
				fmt[1] = format[i];
				strftime(tmp, 32, fmt, &ts);
				buffer_appendstr(buf, tmp);
				break;
			case 'o':
			case 'O':
				strncpy(tmp, get_opponent(game, user), 32);
				for (int j = 0; tmp[j] != 0; j++) {
					if (format[i] == 'o')
						tmp[j] = tolower(tmp[j]);
					else
						tmp[j] = toupper(tmp[j]);
				}
				buffer_appendstr(buf, tmp);
				break;
			case 'u':
				buffer_appendstr(buf, user);
				break;
			case 'U':
				strncpy(tmp, user, 32);
				for (int j = 0; tmp[j] != 0; j++) {
					tmp[j] = toupper(tmp[j]);
				}
				buffer_appendstr(buf, tmp);
				break;
			case 'b':
				{
					double score = json_getdouble(game, "endcontext.score", -1);
					if (score > 0) {
						snprintf(tmp, 32, "%ld", (long)score);
						buffer_appendstr(buf, tmp);
					}
				}
				break;
			case 't':
				{
					double time = json_getdouble(game, "endcontext.finalTime", -1);
					if (time > 0) {
						snprintf(tmp, 32, "%.4f", time/1000.0);
						buffer_appendstr(buf, tmp);
					}
				}
				break;
			case 'T':
				{
					double time = json_getdouble(game, "endcontext.finalTime", -1);
					if (time > 0) {
						int ms = ((int)time) % 1000;
						int s = ((int)time) / 1000;
						int m = s / 60;
						s %= 60;
						snprintf(tmp, 32, "%d'%02d.%03d", m, s, ms);
						buffer_appendstr(buf, tmp);
					}
				}
				break;
			case 'r':
				buffer_appendstr(buf, replayid);
				break;
			case '%':
				buffer_appendchar(buf, '%');
				break;
			default:
				logE("invalid sequence '%%%c' in format", format[i]);
				return NULL;
			}
		} else {
			buffer_appendchar(buf, format[i]);
		}
	}
	return buffer_str(buf);
}

void
download_game(struct json_object_s *game, const char *format, const char *user)
{
	static buffer *b = NULL;
	if (!b) b = buffer_new();
	buffer_truncate(b);

	if (!game)
		return;

	const char *replayid = json_getstring(game, "replayid", 0);
	if (!replayid)
		return;

	const char *filename = generate_filename(game, format, replayid, user);
	if (!filename) {
		logE("failed to generate filename for %s", replayid);
		return;
	}

	if(access(filename, F_OK) != -1 ) {
		logI("Game %s already saved, skipping...", replayid);
		return;
	}

	logI("Downloading %s -> %s... ", replayid, filename);
	if (!ensure_dir(filename)) {
		logE("failed to create/find folder for file!");
	}
	FILE *f = fopen(filename, "wb");
	if (!f) {
		logS("couldnt open output file");
		return;
	}
	char urlbuf[128];
	snprintf(urlbuf, 128, "https://inoue.szy.lol/api/replay/%s", replayid);
	long status;
	long retries = 0;
	for (;;) {
		if (!http_get(urlbuf, b, &status)) {
			fclose(f);
			unlink(filename); // delete the failed file, so the download can be retried
			return;
		}
		if (status != 429 || retries >= 3) {
			break;
		}
		sleep(1 << retries);
		retries++;
	}
	if (status != 200) {
		logE("received error %ld from server: %s", status, buffer_str(b));
		fclose(f);
		unlink(filename);
		return;
	}
	if (!buffer_save(b, f)) {
		logE("Saving failed!");
		fclose(f);
		unlink(filename);
		return;
	}
	total_dl++;
	fclose(f);
}
