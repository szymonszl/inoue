#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "json.h"

#include "inoue.h"

int
main(int argc, char **argv)
{
	printf("INOUE v0.3\n");
	int exitcode = EXIT_FAILURE;

	if (argc == 2) {
		if (chdir(argv[1]) < 0) {
			perror("inoue: couldn't change directory");
			return EXIT_FAILURE;
		}
	}

	if (!http_init()) {
		fprintf(stderr, "failed to start HTTP library, exiting!\n");
		return EXIT_FAILURE;
	}
	exitcode = loadcfg();

	http_deinit();

	return exitcode;
}
