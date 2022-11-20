#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "winunistd.h"
#include "json.h"

#include "inoue.h"

static void
do_dir(const char *dir)
{
	if (dir && chdir(dir) < 0) {
		logS("couldn't change directory to %s", dir);
		return;
	}
	FILE* f = fopen("inoue.cfg", "r");
	if (!f) {
		f = fopen("inoue.txt", "r");
	}
	if (!f) {
		logS("couldn't open config file");
		return;
	}
	buffer *b = buffer_new();
	buffer_load(b, f);
	buffer_appendchar(b, 0);
	fclose(f);
	loadcfg(b);
	buffer_free(b);
}

int
main(int argc, char **argv)
{
	printf("INOUE v0.4\n");

	if (!http_init()) {
		logE("failed to start HTTP library, exiting!");
		return EXIT_FAILURE;
	}

	if (argc == 2) {
		do_dir(argv[1]);
	} else {
		do_dir(".");
	}

	logI("Done!");

	http_deinit();

	if (log_maxseen > LOG_INFO) {
		logE("Errors have occured.");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
