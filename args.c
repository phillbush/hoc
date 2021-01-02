#include <stdlib.h>
#include "hoc.h"
#include "args.h"
#include "error.h"

#define INITARGS 8

Arg *
eallocargs(double d)
{
	Arg *p;

	if ((p = malloc(sizeof *p)) == NULL)
		yyerror("out of memory");
	p->narg = 1;
	p->max = INITARGS;
	if ((p->a = calloc(p->max, sizeof *p->a)) == NULL)
		yyerror("out of memory");
	p->a[0] = d;
	return p;
}

Arg *
addarg(Arg *args, double d)
{
	double *p;
	size_t max;
	
	if (args->narg >= args->max) {
		max = args->max << 1;
		if ((p = realloc(args->a, max * sizeof *p)) == NULL)
			yyerror("out of memory");
		args->max = max;
		args->a = p;
	}
	args->a[args->narg++] = d;
	return args;
}

void
delargs(Arg *args)
{
	if (args) {
		free(args->a);
		free(args);
	}
}
