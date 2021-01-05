#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "hoc.h"
#include "code.h"
#include "error.h"
#include "symbol.h"
#include "gramm.h"

#define FREEAUTO()  { \
	String *tmp, *p = autostrings; \
	while(p) { \
		tmp = p; \
		p = p->next; \
		if (DEBUG) printf("FREE: %s\n", tmp->s); \
		free(tmp->s); \
		free(tmp); \
		autostrings = NULL; \
	} \
}
#define FREEFINAL()  { \
	String *tmp, *p = finalstrings; \
	while(p) { \
		tmp = p; \
		p = p->next; \
		if (DEBUG) printf("FREE: %s\n", tmp->s); \
		free(tmp->s); \
		free(tmp); \
		finalstrings = NULL; \
	} \
}
#define FREESTACK()  { \
	Datum *tmp, *p = stack; \
	while(p) { \
		tmp = p; \
		p = p->next; \
		free(tmp); \
		stack = NULL; \
	} \
}

/* function declaration, needed for bltins[] */
static double Random(void);
static double Integer(double);

/* table of keywords */
static struct {
	char *s;
	int v;
} keywords[] = {
	{"if",          IF},
	{"else",        ELSE},
	{"while",       WHILE},
	{"print",       PRINT},
	{"for",         FOR},
	{"break",       BREAK},
	{"continue",    CONTINUE},
	{NULL,          0}
};

/* table of operations */
static struct {
	char *s;
	void (*f)(void);
} oprs[] = {
	{"oprpop",    oprpop},
	{"eval",      eval},
	{"add",       add},
	{"sub",       sub},
	{"mul",       mul},
	{"mod",       mod},
	{"divd",      divd},
	{"negate",    negate},
	{"power",     power},
	{"assign",    assign},
	{"addeq",     addeq},
	{"subeq",     subeq},
	{"muleq",     muleq},
	{"diveq",     diveq},
	{"modeq",     modeq},
	{"bltin",     bltin},
	{"sympush",   sympush},
	{"constpush", constpush},
	{"print",     print},
	{"prexpr",    prexpr},
	{"gt",        gt},
	{"ge",        ge},
	{"lt",        lt},
	{"le",        le},
	{"eq",        eq},
	{"ne",        ne},
	{"and",       and},
	{"or",        or},
	{"not",       not},
	{"ifcode",    ifcode},
	{"whilecode", whilecode},
	{NULL,        NULL},
};

/* table of bltins functions */
static struct {
	char *s;        /* name */
	int n;          /* arity (-1 = constant) */
	union {
		double d;
		double (*f0)(void);
		double (*f1)(double);
		double (*f2)(double, double);
	} u;
} bltins[] = {
	{"pi",      -1, .u.d = M_PI},
	{"e",       -1, .u.d = M_E},
	{"gamma",   -1, .u.d = 0.57721566490153286060},
	{"deg",     -1, .u.d = 57.29577951308232087680},
	{"phi",     -1, .u.d = 1.61803398874989484820},
	{"rand",    0,  .u.f0 = Random},
	{"int",     1,  .u.f1 = Integer},
	{"abs",     1,  .u.f1 = fabs},
	{"atan",    1,  .u.f1 = atan},
	{"cos",     1,  .u.f1 = cos},
	{"exp",     1,  .u.f1 = exp},
	{"log",     1,  .u.f1 = log},
	{"log10",   1,  .u.f1 = log10},
	{"sin",     1,  .u.f1 = sin},
	{"sqrt",    1,  .u.f1 = sqrt},
	{"atan2",   2,  .u.f2 = atan2},
	{NULL,      0,  .u.d  = 0.0}
};

/* the datum stack */
static Datum *stack = NULL;

/* the machine */
static struct {
	Inst *head;
	Inst *tail;
	Inst *progp;
	Inst *pc;
} prog = {NULL, NULL, NULL, NULL};

/* the string list */
static String *autostrings = NULL;      /* strings freed automatically after execution */
static String *finalstrings = NULL;      /* strings that should be manually freed */

/* flags */
static int breaking, continuing;

/* previously printed value */
static Datum prev = {.next = NULL, .isstr = 0, .u.val = 0.0};

/* add str to String list */
String *
addstr(char *s, int final)
{
	String **list;
	String *p;

	if ((p = malloc(sizeof *p)) == NULL) {
		free(s);
		yyerror("out of memory");
	}
	if (final) {
		list = &finalstrings;
		p->isfinal = 1;
	} else {
		list = &autostrings;
		p->isfinal = 0;
	}
	p->s = s;
	if (*list)
		(*list)->prev = p;
	p->next = *list;
	p->prev = NULL;
	*list = p;
	return p;
}

/* return pointer to operation name */
static char *
oprname(void (*opr)(void))
{
	int i;

	for (i = 0; oprs[i].f; i++)
		if (opr == oprs[i].f)
			return oprs[i].s;
	return "unknown";
}

/* initialize machine */
void
init(void)
{
	int i;

	if ((prog.head = malloc(sizeof *prog.head)) == NULL)
		err(1, "malloc");
	prog.head->next = NULL;
	prog.progp = prog.head;
	srand(time(NULL));
	for (i = 0; keywords[i].s; i++)
		install(keywords[i].s, keywords[i].v);
	for (i = 0; bltins[i].s; i++)
		install(bltins[i].s, BLTIN);
}

/* initialize for code generation */
void
prepare(void)
{
	continuing = breaking = 0;
	prog.tail = NULL;
	prog.progp = prog.head;
	FREEAUTO()
	FREESTACK();
}

/* clean up machine */
void
cleanup(void)
{
	cleansym();
	FREEAUTO()
	FREEFINAL()
	FREESTACK()
}

/* debug the machine */
void
debug(void)
{
	Inst *p;
	size_t n;

	for (n = 0, p = prog.head; p; n++, p = p->next) {
		fprintf(stderr, "%03zu: ", n);
		switch (p->type) {
		case NARG:
			fprintf(stderr, "NARG %d", p->u.narg);
			break;
		case VAL:
			fprintf(stderr, "VAL  %.8g", p->u.val);
			break;
		case SYM:
			fprintf(stderr, "SYM  %s", p->u.sym->name);
			break;
		case OPR:
			fprintf(stderr, "OPR  %s", oprname(p->u.opr));
			break;
		case STR:
			fprintf(stderr, "STR  %s", p->u.str->s);
			break;
		case IP:
			if (p->u.ip == NULL)
				fprintf(stderr, "IP -> NULL");
			else
				fprintf(stderr, "IP -> %03td", n + (p->u.ip - p));
			break;
		}
		fprintf(stderr, "\n");
	}
}

/* run the machine */
void
execute(Inst *ip)
{
	Inst *opc;

	if (ip == NULL)
		prog.pc = prog.head;
	else
		prog.pc = ip;
	while (prog.pc->u.opr && !breaking && !continuing) {
		opc = prog.pc;
		prog.pc = prog.pc->next;
		opc->u.opr();
	}
}

/* install one instruction or operand */
Inst *
code(Inst inst)
{
	Inst *ip;

	ip = prog.progp->next;
	prog.tail = prog.progp;
	*prog.tail = inst;
	prog.tail->next = ip;
	if (!prog.tail->next) {
		if ((ip = malloc(sizeof *ip)) == NULL)
			yyerror("out of memory");
		ip->next = NULL;
		prog.tail->next = ip;
	}
	prog.progp = prog.tail->next;
	return prog.tail;
}

/* get prog.progp */
Inst *
getprogp(void)
{
	return prog.progp;
}

/* push d onto stack */
static void
push(Datum d)
{
	Datum *p;

	if ((p = malloc(sizeof *p)) == NULL)
		yyerror("out of memory");
	*p = d;
	p->next = stack;
	stack = p;
}

/* pop and return top element from stack */
static Datum
pop(void)
{
	Datum *tmp, d;

	if (stack == NULL)
		yyerror("stack underflow");
	tmp = stack;
	d = *stack;
	stack = stack->next;
	free(tmp);
	return d;
}

/* pop top element from stack */
void
oprpop(void)
{
	(void)pop();
}

/* push constant onto stack */
void
constpush(void)
{
	Datum d;

	d.u.val = prog.pc->u.val;
	d.isstr = 0;
	prog.pc = prog.pc->next;
	push(d);
}

/* push previously printed value onto stack */
void
prevpush(void)
{
	push(prev);
}

/* push string onto stack */
void
strpush(void)
{
	Datum d;

	d.u.str = prog.pc->u.str;
	d.isstr = 1;
	prog.pc = prog.pc->next;
	push(d);
}

/* push symbol onto stack */
void
sympush(void)
{
	Datum d;

	d.u.sym = prog.pc->u.sym;
	prog.pc = prog.pc->next;
	push(d);
}

/* free String from datum, String should be listed on finalstrings! */
static void
dfree(String *str)
{
	if (DEBUG)
		printf("FREE: %s\n", str->s);
	free(str->s);
	if (str->next)
		str->next->prev = str->prev;
	if (str->prev)
		str->prev->next = str->next;
	else
		finalstrings = str->next;
	free(str);
}

/* move string from autostrings to finalstrings */
static void
movstr(String *str)
{
	if (str->isfinal)
		return;
	if (str->next)
		str->next->prev = str->prev;
	if (str->prev)
		str->prev->next = str->next;
	else
		autostrings = str->next;
	if (finalstrings)
		finalstrings->prev = str;
	str->next = finalstrings;
	str->prev = NULL;
	str->isfinal = 1;
	finalstrings = str;
}

/* pop numeric value from stack */
static Datum
popnum(int issym)
{
	Datum d;
	double v;

	d = pop();
	if (issym) {
		if (d.u.sym->isstr) {
			v = atof(d.u.sym->u.str->s);
			d.u.sym->u.val = v;
		}
		d.u.sym->isstr = 0;
	} else {
		if (d.isstr) {
			v = atof(d.u.str->s);
			d.u.val = v;
		}
	}
	d.isstr = 0;
	return d;
}

/* add top two elements on stack */
void
add(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val += d2.u.val;
	push(d1);
}

/* subtract top two elements on stack */
void
sub(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val -= d2.u.val;
	push(d1);
}

/* multiply top two elements on stack */
void
mul(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val *= d2.u.val;
	push(d1);
}

/* divide top two elements on stack */
void
divd(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	if (d2.u.val == 0.0)
		yyerror("division by zero");
	d1 = popnum(0);
	d1.u.val /= d2.u.val;
	push(d1);
}

/* compute module of top two elements on stack */
void
mod(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	if (d2.u.val == 0.0)
		yyerror("module by zero");
	d1 = popnum(0);
	d1.u.val = fmod(d1.u.val, d2.u.val);
	push(d1);
}

/* negate top element on stack */
void
negate(void)
{
	Datum d;

	d = popnum(0);
	d.u.val = -d.u.val;
	push(d);
}

/* compute power of top two elements on stack */
void
power(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val = pow(d1.u.val, d2.u.val);
	push(d1);
}

/* verify whether symbol is valid variable for evaluation */
static void
verifyeval(Symbol *s)
{
	if (s->type != VAR && s->type != UNDEF)
		yyerror("attempt to evaluate non-variable: %s", s->name);
	if (s->type == UNDEF)
		yyerror("undefined variable: %s", s->name);
}

/* evaluate variable on stack */
void
eval(void)
{
	Datum d;

	d = pop();
	verifyeval(d.u.sym);
	d.isstr = d.u.sym->isstr;
	d.u = d.u.sym->u;
	push(d);
}

/* get sym from pc and cast it into a number; and increment pc */
static Datum
dsymnum(void)
{
	Datum d;
	double v;

	d.u.sym = prog.pc->u.sym;
	prog.pc = prog.pc->next;
	if (d.u.sym->isstr) {
		v = atof(d.u.sym->u.str->s);
		if (d.u.sym->isstr && prev.isstr && d.u.sym->u.str != prev.u.str)
			dfree(d.u.sym->u.str);
		d.u.sym->u.val = v;
		d.u.sym->isstr = 0;
	}
	d.isstr = 0;
	return d;
}

/* pre-increment variable */
void
preinc(void)
{
	Datum d;

	d = dsymnum();
	verifyeval(d.u.sym);
	d.u.val = d.u.sym->u.val += 1.0;
	push(d);
}

/* pre-decrement variable */
void
predec(void)
{
	Datum d;

	d = dsymnum();
	verifyeval(d.u.sym);
	d.u.val = d.u.sym->u.val -= 1.0;
	push(d);
}

/* post-increment variable */
void
postinc(void)
{
	Datum d;
	double v;

	d = dsymnum();
	verifyeval(d.u.sym);
	v = d.u.sym->u.val;
	d.u.sym->u.val += 1.0;
	d.u.val = v;
	push(d);
}

/* post-decrement variable */
void
postdec(void)
{
	Datum d;
	double v;

	d = dsymnum();
	verifyeval(d.u.sym);
	v = d.u.sym->u.val;
	d.u.sym->u.val -= 1.0;
	d.u.val = v;
	push(d);
}

static void
verifyassign(Symbol *s)
{
	if (s->type != VAR && s->type != UNDEF)
		yyerror("assignment to non-variable: %s", s->name);
}

/* assign top value to next value */
void
assign(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.u.sym);
	if (d1.u.sym->isstr && prev.isstr && d1.u.sym->u.str != prev.u.str)
		dfree(d1.u.sym->u.str);
	d1.u.sym->isstr = d2.isstr;
	if (d2.isstr)
		movstr(d2.u.str);
	d1.u.sym->u = d2.u;
	d1.u.sym->isstr = d2.isstr;
	d1.u.sym->type = VAR;
	push(d2);
}

/* add and assign top value to next value */
void
addeq(void)
{
	Datum d1, d2;

	d1 = popnum(1);
	d2 = popnum(0);
	verifyassign(d1.u.sym);
	d2.u.val = d1.u.sym->u.val += d2.u.val;
	d1.u.sym->type = VAR;
	push(d2);
}

/* subtract and assign top value to next value */
void
subeq(void)
{
	Datum d1, d2;

	d1 = popnum(1);
	d2 = popnum(0);
	verifyassign(d1.u.sym);
	d2.u.val = d1.u.sym->u.val -= d2.u.val;
	d1.u.sym->type = VAR;
	push(d2);
}

/* multiply and assign top value to next value */
void
muleq(void)
{
	Datum d1, d2;

	d1 = popnum(1);
	d2 = popnum(0);
	verifyassign(d1.u.sym);
	d2.u.val = d1.u.sym->u.val *= d2.u.val;
	d1.u.sym->type = VAR;
	push(d2);
}

/* divide and assign top value to next value */
void
diveq(void)
{
	Datum d1, d2;

	d1 = popnum(1);
	d2 = popnum(0);
	verifyassign(d1.u.sym);
	d2.u.val = d1.u.sym->u.val /= d2.u.val;
	d1.u.sym->type = VAR;
	push(d2);
}

/* compute module and assign top value to next value */
void
modeq(void)
{
	Datum d1, d2;
	long n;

	d1 = popnum(1);
	d2 = popnum(0);
	verifyassign(d1.u.sym);
	n = d1.u.sym->u.val;
	n %= (long)d2.u.val;
	d2.u.val = d1.u.sym->u.val = n;
	d1.u.sym->type = VAR;
	push(d2);
}

/* pop top value from stack, print it */
static Datum
pr(void)
{
	Datum d;

	d = pop();
	if (d.isstr)
		printf("%s\n", d.u.str->s);
	else
		printf("%.8g\n", d.u.val);
	return d;
}

/* duplicate datum */
static Datum
ddup(String *str)
{
	Datum d;
	char *s;

	if ((s = strdup(str->s)) == NULL)
		yyerror("out of memory");
	d.u.str = addstr(s, 1);
	d.isstr = 1;
	return d;
}

void
print(void)
{
	Datum d;

	d = pr();
	if (d.isstr) {
		if (prev.isstr && d.u.str != prev.u.str)
			dfree(prev.u.str);
		if (d.u.str->isfinal && (!prev.isstr || prev.u.str != d.u.str))
			d = ddup(d.u.str);
		else
			movstr(d.u.str);
	} else if (prev.isstr) {
		dfree(prev.u.str);
	}
	prev = d;
}

void
prexpr(void)
{
	(void)pr();
}

void
gt(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val = (double)(d1.u.val > d2.u.val);
	push(d1);
}

void
ge(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val = (double)(d1.u.val >= d2.u.val);
	push(d1);
}

void
lt(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val = (double)(d1.u.val < d2.u.val);
	push(d1);
}

void
le(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val = (double)(d1.u.val <= d2.u.val);
	push(d1);
}

void
eq(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val = (double)(d1.u.val == d2.u.val);
	push(d1);
}

void
ne(void)
{
	Datum d1, d2;

	d2 = popnum(0);
	d1 = popnum(0);
	d1.u.val = (double)(d1.u.val != d2.u.val);
	push(d1);
}

void
not(void)
{
	Datum d;

	d = popnum(0);
	d.u.val = (double)(!d.u.val);
	push(d);
}

void
and(void)
{
	Datum d;
	Inst *savepc;

	savepc = prog.pc;
	d = popnum(0);
	if (d.u.val) {
		execute(savepc->u.ip);
		d = popnum(0);
	}
	d.u.val = d.u.val ? 1.0 : 0.0;
	push(d);
	prog.pc = N1(savepc)->u.ip;
}

void
or(void)
{
	Datum d;
	Inst *savepc;

	savepc = prog.pc;
	d = popnum(0);
	if (!d.u.val) {
		execute(savepc->u.ip);
		d = popnum(0);
	}
	d.u.val = d.u.val ? 1.0 : 0.0;
	push(d);
	prog.pc = N1(savepc)->u.ip;
}

static double
cond(Inst *pc)
{
	Datum d;

	if (pc == NULL)
		return 1.0;
	execute(pc);
	d = popnum(0);
	return d.u.val;
}

static Datum
execpop(Inst *pc)
{
	Datum d;

	if (pc && pc->u.opr) {
		execute(pc);
		d = popnum(0);
	} else {
		d.u.val = 0.0;
	}
	return d;
}

void
ifcode(void)
{
	Datum d;
	Inst *savepc;

	savepc = prog.pc;                       /* then part */
	d = execpop(N3(savepc));
	if (d.u.val)
		execute(savepc->u.ip);
	else if (N1(savepc)->u.ip)              /* else part? */
		execute(N1(savepc)->u.ip);
	prog.pc = N2(savepc)->u.ip;             /* next statement */
}

void
whilecode(void)
{
	Inst *savepc;

	savepc = prog.pc;
	while (cond(N2(savepc))) {
		execute(savepc->u.ip);
		if (continuing) {
			continuing = 0;
			continue;
		}
		if (breaking) {
			breaking = 0;
			break;
		}
	}
	prog.pc = N1(savepc)->u.ip;
}

void
forcode(void)
{
	Inst *savepc;

	savepc = prog.pc;
	for ((void)execpop(N4(savepc)); cond(savepc->u.ip); (void)execpop(N1(savepc)->u.ip)) {
		execute(N2(savepc)->u.ip);
		if (continuing) {
			continuing = 0;
			continue;
		}
		if (breaking) {
			breaking = 0;
			break;
		}
	}
	prog.pc = N3(savepc)->u.ip;
}

void
breakcode(void)
{
	breaking = 1;
}

void
continuecode(void)
{
	continuing = 1;
}

/* get random from 0 to 1 */
static double
Random(void)
{
	return (double)rand() / RAND_MAX;
}

/* get integer part of double */
static double
Integer(double n)
{
	return (double)(long)n;
}

/* check result of library call */
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

/* evaluate built-in on top of stack */
void
bltin(void)
{
	Datum d1, d2;
	Symbol *sym;
	int narg, i;

	sym = prog.pc->u.sym;
	prog.pc = prog.pc->next;
	narg = prog.pc->u.narg;
	prog.pc = prog.pc->next;
	for (i = 0; bltins[i].s; i++)
		if (strcmp(sym->name, bltins[i].s) == 0)
			break;
	if (narg == 0 && bltins[i].n == -1)
		narg = -1;
	if (narg != bltins[i].n)
		yyerror("%s: wrong arity", bltins[i].s);
	errno = 0;
	switch (narg) {
	case -1:
		d1.u.val = bltins[i].u.d;
		break;
	case 0:
		d1.u.val = (*bltins[i].u.f0)();
		break;
	case 1:
		d1 = popnum(0);
		d1.u.val = (*bltins[i].u.f1)(d1.u.val);
		break;
	case 2:
		d2 = popnum(0);
		d1 = popnum(0);
		d1.u.val = (*bltins[i].u.f2)(d1.u.val, d2.u.val);
		break;
	}
	d1.u.val = errcheck(d1.u.val, bltins[i].s);
	d1.isstr = 0;
	push(d1);
}
