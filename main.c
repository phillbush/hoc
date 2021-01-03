#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include "error.h"
#include "symbol.h"
#include "code.h"
#include "gramm.h"

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
	yyerror("floating point exception");
}

/* hoc */
int
main(int argc, char *argv[])
{
	struct sigaction sa;
	FILE *fp = NULL;

	if (argc > 2 || (argc == 2 && argv[1][0] == '-' && argv[1][1]))
		usage();

	/* assign action for SIGFPE */
	sa.sa_handler = sigfpehand;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGFPE, &sa, NULL) == -1)
		err(1, "sigaction");

	/* open input file */
	if (argc == 2 && strcmp(argv[1], "-") != 0)
		if ((fp = fopen(argv[1], "r")) == NULL)
			err(1, "%s", argv[1]);
	if (fp)
		yyin = fp;

	/* install keywords and bultin functions */
	init();

	/* parse and execute input until EOF */
	setjmp(begin);
	while (initcode(), yyparse()) {
		if (DEBUG)
			debug();
		execute(NULL);
	}

	/* close input file and clear symbol table */
	if (fp)
		fclose(fp);
	cleansym();

	return 0;
}
