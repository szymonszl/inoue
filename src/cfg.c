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
	TSK_ZENITH,
	TSK_ZENITHEX,
};

static void
dispatch(char *user, char *format, enum tasktype type)
{
	if (!user) {
		logE("config: no user selected!");
	}
	const char *gamemode = NULL;
	const char *proper_ext = "";
	// enum { TOP = 1, RECENT = 2, PROGRESSION = 4 } lbs = 0;
	switch (type) {
		case TSK_NONE:
			logE("config: no task type selected!");
			return;
		case TSK_40L:
			if (!format) format = "%Y-%m-%d_%H-%M_%T.ttr";
			proper_ext = ".ttr";
			gamemode = "40l";
			// lbs = TOP|RECENT|PROGRESSION;
			logI("downloading 40L from %s...", user);
			break;
		case TSK_BLITZ:
			if (!format) format = "%Y-%m-%d_%H-%M_%b.ttr";
			proper_ext = ".ttr";
			gamemode = "blitz";
			// lbs = TOP|RECENT|PROGRESSION;
			logI("downloading Blitz from %s...", user);
			break;
		case TSK_LEAGUE:
			if (!format) format = "%Y-%m-%d_%H-%M_%O.ttrm";
			proper_ext = ".ttrm";
			gamemode = "league";
			// lbs = RECENT;
			logI("downloading Tetra League from %s...", user);
			break;
		case TSK_ZENITH:
			if (!format) format = "%Y-%m-%d_%H-%M_%a.ttrm";
			proper_ext = ".ttr";
			gamemode = "zenith";
			// lbs = RECENT;
			logI("downloading Quick Play from %s...", user);
			break;
		case TSK_ZENITHEX:
			if (!format) format = "%Y-%m-%d_%H-%M_%a.ttrm";
			proper_ext = ".ttr";
			gamemode = "zenithex";
			// lbs = RECENT;
			logI("downloading Expert Quick Play from %s...", user);
			break;
	}
	if (!endswith(format, proper_ext)) {
		const char *found = strrchr(format, '.');
		logW("wrong or no extension in format (expected %s, got %s)", proper_ext, found?found:"");
	}
	/*
	if (lbs & TOP)
		download_leaderboard(format, user, gamemode, "top");
	if (lbs & RECENT)
		download_leaderboard(format, user, gamemode, "recent");
	if (lbs & PROGRESSION)
		download_leaderboard(format, user, gamemode, "progression");
	*/ // this won't work for now
	download_leaderboard(format, user, gamemode, "recent");
}

enum cfgword {
	CFG_NONE = 0,

	CFG_USER,
	CFG_FORMAT,
	CFG_40L,
	CFG_BLITZ,
	CFG_LEAGUE,

	CFG_ZENITH,
	CFG_ZENITHEX,
	CFG_NEXT,
}; // order has to match words[] below!

static enum cfgword
read_word(const char *c, const char **n)
{
	const char *words[] = {
		"user", "saveas", "40l", "blitz", "league",
		"qp", "qpexpert", "also", NULL
	};
	for (int i = 0; words[i] != NULL; i++) {
		int l = strlen(words[i]);
		if (memcmp(c, words[i], l) == 0 && (isspace(c[l]) || c[l] == '\0')) {
			if (n) {
				if (c[l]) {
					*n = &c[l+1];
				} else { // immediate null byte after
					*n = &c[l];
				}
			}
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

void
loadcfg(const char *c)
{
	char *user = NULL;
	char *format = NULL;
	enum tasktype type = TSK_NONE;
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
			return;
		}
		c = next;

#define READ_VALUE(__var) \
	if (__var) free(__var); \
	__var = read_string(&c); \
	if (!__var) return;

		const char *type_err = "config: you can only download one type of replay at the same time";
#define CHECK_TYPE() if (type) { logE(type_err); return; }

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
			case CFG_ZENITH:
				CHECK_TYPE();
				type = TSK_ZENITH;
				break;
			case CFG_ZENITHEX:
				CHECK_TYPE();
				type = TSK_ZENITHEX;
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
}