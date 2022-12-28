#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "winunistd.h"
#include "json.h"

#include "inoue.h"

int opt_quiet = 0; // extern definition
int total_dl = 0; // extern definition

static void
do_dir(const char *dir)
{
	char *cwd;
	if ((cwd = getcwd_()) == NULL)
		return;
	if (dir && chdir(dir) < 0) {
		logS("couldn't change directory to %s", dir);
		return;
	}
	FILE* f = fopen("inoue.cfg", "r");
	if (!f) {
		f = fopen("inoue.txt", "r");
	}
	if (!f) {
		logS("couldn't open config file at %s/%s", cwd, dir);
		return;
	}
	buffer *b = buffer_new();
	buffer_load(b, f);
	buffer_appendchar(b, 0);
	fclose(f);
	loadcfg(buffer_str(b));
	buffer_free(b);
	chdir(cwd);
}

int
main(int argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][2] == '\0') {
			switch (argv[i][1]) {
				case 'q':
					opt_quiet = 1;
					break;
				case 'V':
					puts("Inoue " INOUE_VER);
					return EXIT_SUCCESS;
				case 'h':
					fprintf(stderr, "Usage: %s [-h] [-q] [[-c <command> | <path>]...]\n", argv[0]);
					return EXIT_SUCCESS;
				case 'c':
					i++; // ignore for now
					break;
				default:
					logE("unknown parameter -%c, try -h", argv[i][1]);
					return EXIT_FAILURE;
			}
		}
	}

	logI("starting inoue " INOUE_VER);

	if (!http_init()) {
		logE("failed to start HTTP library, exiting!");
		return EXIT_FAILURE;
	}

	int had_cfgs = 0;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][2] == '\0') {
			switch (argv[i][1]) {
				case 'q':
				case 'V':
				case 'h':
					break;
				case 'c':
					i++;
					if (i < argc && argv[i]) {
						loadcfg(argv[i]);
					} else {
						logE("config string required for -c (none provided)");
					}
					had_cfgs = 1;
					break;
			}
		} else {
			do_dir(argv[i]);
			had_cfgs = 1;
		}
	}
	if (!had_cfgs) {
		do_dir(".");
	}

	logI("Done!");

	http_deinit();

	if (!opt_quiet) {
		logI("Downloaded %d replays total.", total_dl);
		if (log_maxseen > LOG_INFO) {
			// safe to put here, no risk of errors getting spammed away in quiet mode
			logE("Errors have occured.");
		}
	}

#ifdef _WIN32
	// for console window business on windowed platform, pass -q to disable
	if (!opt_quiet) { puts("Press any key to exit..."); getch(); }
#endif

	return (log_maxseen > LOG_INFO) ? EXIT_FAILURE : EXIT_SUCCESS;
}
