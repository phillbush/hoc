#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include "hoc.h"
#include "code.h"

extern FILE *yyin;
jmp_buf begin;

int yyparse(void);

/* show usage */
static void
usage(void)
{
	(void)fprintf(stderr, "usage: hoc [file]\n");
	exit(1);
}

/* catch floating point exceptions */
static void
sigfpehand(int sig)
{
	(void)sig;
	err(1, "floating point exception");
}

/* hoc */
int
main(int argc, char *argv[])
{
	struct sigaction sa;
	FILE *fp = NULL;
	char ch;

	while ((ch = getopt(argc, argv, "")) != -1) {
		switch (ch) {
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	/* assign action for SIGFPE */
	sa.sa_handler = sigfpehand;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGFPE, &sa, NULL) == -1)
		err(1, "sigaction");

	/* open input file */
	if (argc) {
		if (strcmp(*argv, "-") != 0)
			if ((fp = fopen(*argv, "r")) == NULL)
				err(1, "%s", *argv);
		if (fp)
			yyin = fp;
	}

	/* initialize machine */
	init(argc, argv);

	/* parse and execute input until EOF */
	setjmp(begin);
	while (prepare(), yyparse()) {
		if (DEBUG)
			debug();
		execute(NULL);
	}

	/* cleanup machine and close input file */
	cleanup();
	if (fp)
		fclose(fp);

	return 0;
}
