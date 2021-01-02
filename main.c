#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include "hoc.h"
#include "code.h"
#include "error.h"
#include "symbol.h"
#include "bltin.h"
#include "y.tab.h"

#ifndef DEBUG
#define DEBUG 0
#endif

extern FILE *yyin;
jmp_buf begin;

int yyparse(void);

/* keywords */
static struct {
	char *s;
	int v;
} keywords[] = {
	{"if",      IF},
	{"else",    ELSE},
	{"while",   WHILE},
	{"print",   PRINT},
	{"for",     FOR},
	{NULL,      0},
};

static void
usage(void)
{
	(void)fprintf(stderr, "usage: hoc [file]\n");
	exit(1);
}

static void
sigfpehand(int sig)
{
	(void)sig;
	yyerror("floating point exception");
}

int
main(int argc, char *argv[])
{
	struct sigaction sa;
	size_t i;
	FILE *fp = NULL;

	if (argc > 2 || (argc == 2 && argv[1][0] == '-' && argv[1][1]))
		usage();

	/* assign action for SIGFPE */
	sa.sa_handler = sigfpehand;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGFPE, &sa, NULL) == -1)
		err(1, "sigaction");

	if (argc == 2 && strcmp(argv[1], "-") != 0)
		if ((fp = fopen(argv[1], "r")) == NULL)
			err(1, "%s", argv[1]);
	if (fp)
		yyin = fp;

	/* install keywords and bultin functions */
	for (i = 0; keywords[i].s; i++)
		install(keywords[i].s, keywords[i].v, 0.0, NULL);
	for (i = 0; bltins[i].s; i++)
		install(bltins[i].s, BLTIN, 0.0, bltins[i].f);

	/* parse and execute input */
	setjmp(begin);
	while (initcode(), yyparse()) {
		if (DEBUG)
			debug();
		execute(prog);
	}

	/* close file and clear symbol table */
	if (fp)
		fclose(fp);
	cleansym();

	return 0;
}
