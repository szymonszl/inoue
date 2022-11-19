#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "inoue.h"

enum tasktype {
	TSK_NONE = 0,
	TSK_40L,
	TSK_BLITZ,
	TSK_LEAGUE,
};

static void
dispatch(char *user, char *format, enum tasktype type)
{
	const char *uid = resolve_username(user);
	if (!uid) {
		logE("failed to resolve user %s!", user);
		return;
	}
	const char *urlfmt1, *urlfmt2 = NULL;
	switch (type) {
		case TSK_NONE:
			logE("config: no task type selected!");
			return;
		case TSK_40L:
			if (!format) format = "%Y-%m-%d_%H-%M_%T.ttr";
			if (!endswith(format, ".ttr"))
				logW("wrong or no extension for singleplayer (expected .ttr)");
			urlfmt1 = "https://ch.tetr.io/api/streams/40l_userrecent_%s";
			urlfmt2 = "https://ch.tetr.io/api/streams/40l_userbest_%s";
			logI("downloading 40L from %s...", user);
			break;
		case TSK_BLITZ:
			if (!format) format = "%Y-%m-%d_%H-%M_%b.ttr";
			if (!endswith(format, ".ttr"))
				logW("wrong or no extension for singleplayer (expected .ttr)");
			urlfmt1 = "https://ch.tetr.io/api/streams/blitz_userrecent_%s";
			urlfmt2 = "https://ch.tetr.io/api/streams/blitz_userbest_%s";
			logI("downloading Blitz from %s...", user);
			break;
		case TSK_LEAGUE:
			if (!format) format = "%Y-%m-%d_%H-%M_%O.ttrm";
			if (!endswith(format, ".ttrm"))
				logW("wrong or no extension for multiplayer (expected .ttrm)");
			urlfmt1 = "https://ch.tetr.io/api/streams/league_userrecent_%s";
			logI("downloading Tetra League from %s...", user);
			break;
	}
	char urlbuf[128];
	snprintf(urlbuf, 128, urlfmt1, uid);
	download_from_stream(urlbuf, format, user);
	if (urlfmt2) {
		snprintf(urlbuf, 128, urlfmt2, uid);
		download_from_stream(urlbuf, format, user);
	}
}

enum cfgword {
	CFG_NONE = 0,
	CFG_USER,
	CFG_FORMAT,
	CFG_40L,
	CFG_BLITZ,
	CFG_LEAGUE,
	CFG_NEXT,
};

static enum cfgword
read_word(const char *c, const char **n)
{
	const char *words[] = {
		"user", "saveas", "40l", "blitz", "league", "also", NULL
	};
	for (int i = 0; words[i] != NULL; i++) {
		int l = strlen(words[i]);
		if (memcmp(c, words[i], l) == 0 && (isspace(c[l]) || c[l] == '\0')) {
			if (n)
				*n = &c[l+1];
			return i+1;
		}
	}
	return CFG_NONE;
}

char *
read_string(const char **_c)
{
	const char *c = *_c;
	while (isspace(*c) && *c) c++;
	if (!c) {
		logE("config: expected value");
		return NULL;
	}
	char buf[1024];
	int bufc = 0;
	if (*c == '"') { // quoted
		c++;
		for (;;) {
			char ch = *c;
			if (!ch) {
				logE("config: no matching quote while reading value");
				return NULL;
			}
			if (ch == '\\' && *(c+1)) {
				buf[bufc++] = *(c+1);
				c += 2;
				continue;
			}
			if (ch == '"') {
				buf[bufc] = 0;
				*_c = c+1;
				return strdup(buf);
			}
			buf[bufc++] = ch;
			c++;
		}
	} else { // unquoted
		for (;;) {
			char ch = *c;
			if (isspace(ch) || !ch) {
				if (bufc) {
					buf[bufc] = 0;
					*_c = c;
					return strdup(buf);
				} else {
					logE("config: empty value?");
					return NULL;
				}
			}
			buf[bufc++] = ch;
			c++;
		}
	}
}

int
loadcfg(void)
{
	FILE* f = fopen("inoue.cfg", "r");
	if (!f) {
		logS("couldnt open config file");
		return 0;
	}
	buffer *b = buffer_new();
	buffer_load(b, f);
	buffer_appendchar(b, 0);
	fclose(f);
	const char *c = buffer_str(b);
	char *user = NULL;
	char *format = NULL;
	enum tasktype type = TSK_NONE;
	int err = 1;
	for (;;) {
		while (isspace(*c) && *c) c++;
		if (*c == '#') {
			for (;;) {
				char ch = *c;
				if (!ch) break;
				c++;
				if (ch == '\n') {
					break;
				}
			}
			continue;
		}
		if (!*c)
			break; // eof
		const char *next = NULL;
		enum cfgword word = read_word(c, &next);
		if (word == CFG_NONE) {
			logE("config: unrecognized word at '%.10s'...", c);
			goto exit;
		}
		c = next;

#define READ_VALUE(__var) \
	if (__var) free(__var); \
	__var = read_string(&c); \
	if (!__var) goto exit;

		const char *type_err = "config: you can only download one type of replay at the same time";
#define CHECK_TYPE() if (type) { logE(type_err); goto exit; }

		switch (word) {
			case CFG_NONE:
				break; // unreachable
			case CFG_USER:
				READ_VALUE(user);
				break;
			case CFG_FORMAT:
				READ_VALUE(format);
				break;
			case CFG_40L:
				CHECK_TYPE();
				type = TSK_40L;
				break;
			case CFG_BLITZ:
				CHECK_TYPE();
				type = TSK_BLITZ;
				break;
			case CFG_LEAGUE:
				CHECK_TYPE();
				type = TSK_LEAGUE;
				break;
			case CFG_NEXT:
				dispatch(user, format, type);
				type = 0;
				if (format) {
					free(format);
					format = NULL;
				}
				break;
		}
	}
	dispatch(user, format, type);
	err = 0;
exit:
	buffer_free(b);
	return err;
}