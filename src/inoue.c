#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "winunistd.h"
#include "json.h"

#include "inoue.h"

int
main(int argc, char **argv)
{
	printf("INOUE v0.4\n");

	if (argc == 2) {
		if (chdir(argv[1]) < 0) {
			logS("couldn't change directory to %s", argv[1]);
			return EXIT_FAILURE;
		}
	}

	if (!http_init()) {
		logE("failed to start HTTP library, exiting!");
		return EXIT_FAILURE;
	}
	loadcfg();

	http_deinit();

	return log_maxseen > LOG_INFO;
}
