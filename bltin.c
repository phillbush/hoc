#include <math.h>
#include <errno.h>
#include "hoc.h"
#include "bltin.h"
#include "error.h"

/* built-ins */
struct Bltin_name bltins[] = {
	{"pi",      Pi},
	{"e",       E},
	{"gamma",   Gamma},
	{"deg",     Deg},
	{"phi",     Phi},
	{"abs",     Abs},
	{"atan",    Atan},
	{"atan2",   Atan2},
	{"cos",     Cos},
	{"exp",     Exp},
	{"int",     Integer},
	{"log",     Log},
	{"log10",   Log10},
	{"sin",     Sin},
	{"sqrt",    Sqrt},
	{NULL,      NULL}
};

static void
aritycheck(Arg *a, const char *s, size_t n)
{
	if (a == NULL)
		yyerror("%s: no argument structure", s);
	if (a->narg != n)
		yyerror("%s: wrong arity", s);
}

static double
errcheck(double d, const char *s)
{
	if (errno == EDOM) {
		errno = 0;
		yyerror("%s: argument out of domain", s);
	}
	if (errno == ERANGE) {
		errno = 0;
		yyerror("%s: result out of range", s);
	}
	return d;
}

double
Pi(Arg *a)
{
	(void)a;
	return M_PI;
}

double
E(Arg *a)
{
	(void)a;
	return M_E;
}

/* Euler */
double
Gamma(Arg *a)
{
	(void)a;
	return 0.57721566490153286060;
}

/* deg/radian */
double
Deg(Arg *a)
{
	(void)a;
	return 57.29577951308232087680;
}

/* golden ratio */
double
Phi(Arg *a)
{
	(void)a;
	return 1.61803398874989484820;
}

double
Abs(Arg *a)
{
	aritycheck(a, "Abs", 1);
	return fabs(a->a[0]);
}

double
Atan(Arg *a)
{
	aritycheck(a, "atan", 1);
	return atan(a->a[0]);
}

double
Atan2(Arg *a)
{
	aritycheck(a, "atan2", 2);
	return atan2(a->a[0], a->a[1]);
}

double
Cos(Arg *a)
{
	aritycheck(a, "cos", 1);
	return cos(a->a[0]);
}

double
Exp(Arg *a)
{
	aritycheck(a, "exp", 1);
	return errcheck(exp(a->a[0]), "exp");
}

double
Integer(Arg *a)
{
	aritycheck(a, "exp", 1);
	return (double)(long)a->a[0];
}

double
Log(Arg *a)
{
	aritycheck(a, "log", 1);
	return errcheck(log(a->a[0]), "log");
}

double
Log10(Arg *a)
{
	aritycheck(a, "log10", 1);
	return errcheck(log10(a->a[0]), "log");
}

double
Sin(Arg *a)
{
	aritycheck(a, "sin", 1);
	return sin(a->a[0]);
}

double
Sqrt(Arg *a)
{
	aritycheck(a, "sqrt", 1);
	return errcheck(sqrt(a->a[0]), "sqrt");
}
