#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "error.h"
#include "symbol.h"
#include "code.h"
#include "gramm.h"

/* function declaration, needed for bltins[] */
static double Random(void);
static double Integer(double);

/* keywords */
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

/* operations */
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

/* bltins */
static struct {
	char *s;
	int n;
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

/* datum stack */
static Datum *stack = NULL;

/* program queue */
static struct {
	Inst *head;
	Inst *tail;
	Inst *progp;
	Inst *pc;
} prog = {NULL, NULL, NULL, NULL};

/* flags */
static int breaking, continuing;

/* previously printed value */
static double prev = 0;

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

/* initiate machine */
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
		install(keywords[i].s, keywords[i].v, 0.0);
	for (i = 0; bltins[i].s; i++)
		install(bltins[i].s, BLTIN, 0.0);
}

/* initialize for code generation */
void
initcode(void)
{
	continuing = breaking = 0;
	prog.tail = NULL;
	prog.progp = prog.head;
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

/* pop and return top element from stack */
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
	prog.pc = prog.pc->next;
	push(d);
}

/* push previously printed value onto stack */
void
prevpush(void)
{
	Datum d;

	d.u.val = prev;
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

/* add top two elements on stack */
void
add(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.u.val += d2.u.val;
	push(d1);
}

/* subtract top two elements on stack */
void
sub(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.u.val -= d2.u.val;
	push(d1);
}

/* multiply top two elements on stack */
void
mul(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.u.val *= d2.u.val;
	push(d1);
}

/* divide top two elements on stack */
void
divd(void)
{
	Datum d1, d2;

	d2 = pop();
	if (d2.u.val == 0.0)
		yyerror("division by zero");
	d1 = pop();
	d1.u.val /= d2.u.val;
	push(d1);
}

/* compute module of top two elements on stack */
void
mod(void)
{
	Datum d1, d2;

	d2 = pop();
	if (d2.u.val == 0.0)
		yyerror("module by zero");
	d1 = pop();
	d1.u.val = fmod(d1.u.val, d2.u.val);
	push(d1);
}

/* negate top element on stack */
void
negate(void)
{
	Datum d;

	d = pop();
	d.u.val = -d.u.val;
	push(d);
}

/* compute power of top two elements on stack */
void
power(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
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
	d.u.val = d.u.sym->val;
	push(d);
}

/* pre-increment variable */
void
preinc(void)
{
	Datum d;

	d.u.sym = prog.pc->u.sym;
	prog.pc = prog.pc->next;
	verifyeval(d.u.sym);
	d.u.val = d.u.sym->val += 1.0;
	push(d);
}

/* pre-decrement variable */
void
predec(void)
{
	Datum d;

	d.u.sym = prog.pc->u.sym;
	prog.pc = prog.pc->next;
	verifyeval(d.u.sym);
	d.u.val = d.u.sym->val -= 1.0;
	push(d);
}

/* post-increment variable */
void
postinc(void)
{
	Datum d;
	double v;

	d.u.sym = prog.pc->u.sym;
	prog.pc = prog.pc->next;
	verifyeval(d.u.sym);
	v = d.u.sym->val;
	d.u.sym->val += 1.0;
	d.u.val = v;
	push(d);
}

/* post-decrement variable */
void
postdec(void)
{
	Datum d;
	double v;

	d.u.sym = prog.pc->u.sym;
	prog.pc = prog.pc->next;
	verifyeval(d.u.sym);
	v = d.u.sym->val;
	d.u.sym->val -= 1.0;
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
	d1.u.sym->val = d2.u.val;
	d1.u.sym->type = VAR;
	push(d2);
}

/* add and assign top value to next value */
void
addeq(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.u.sym);
	d2.u.val = d1.u.sym->val += d2.u.val;
	d1.u.sym->type = VAR;
	push(d2);
}

/* subtract and assign top value to next value */
void
subeq(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.u.sym);
	d2.u.val = d1.u.sym->val -= d2.u.val;
	d1.u.sym->type = VAR;
	push(d2);
}

/* multiply and assign top value to next value */
void
muleq(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.u.sym);
	d2.u.val = d1.u.sym->val *= d2.u.val;
	d1.u.sym->type = VAR;
	push(d2);
}

/* divide and assign top value to next value */
void
diveq(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.u.sym);
	d2.u.val = d1.u.sym->val /= d2.u.val;
	d1.u.sym->type = VAR;
	push(d2);
}

/* compute module and assign top value to next value */
void
modeq(void)
{
	Datum d1, d2;
	long n;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.u.sym);
	n = d1.u.sym->val;
	n %= (long)d2.u.val;
	d2.u.val = d1.u.sym->val = n;
	d1.u.sym->type = VAR;
	push(d2);
}

/* pop top value from stack, print it */
void
print(void)
{
	Datum d;

	d = pop();
	printf("%.8g\n", d.u.val);
	prev = d.u.val;
}

void
prexpr(void)
{
	Datum d;

	d = pop();
	printf("%.8g\n", d.u.val);
}

void
gt(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.u.val = (double)(d1.u.val > d2.u.val);
	push(d1);
}

void
ge(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.u.val = (double)(d1.u.val >= d2.u.val);
	push(d1);
}

void
lt(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.u.val = (double)(d1.u.val < d2.u.val);
	push(d1);
}

void
le(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.u.val = (double)(d1.u.val <= d2.u.val);
	push(d1);
}

void
eq(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.u.val = (double)(d1.u.val == d2.u.val);
	push(d1);
}

void
ne(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.u.val = (double)(d1.u.val != d2.u.val);
	push(d1);
}

void
not(void)
{
	Datum d;

	d = pop();
	d.u.val = (double)(!d.u.val);
	push(d);
}

void
and(void)
{
	Datum d;
	Inst *savepc;

	savepc = prog.pc;
	d = pop();
	if (d.u.val) {
		execute(savepc->u.ip);
		d = pop();
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
	d = pop();
	if (!d.u.val) {
		execute(savepc->u.ip);
		d = pop();
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
	d = pop();
	return d.u.val;
}

static Datum
execpop(Inst *pc)
{
	Datum d;

	if (pc && pc->u.opr) {
		execute(pc);
		d = pop();
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
	for (execpop(N4(savepc)); cond(savepc->u.ip); execpop(N1(savepc)->u.ip)) {
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
		d1 = pop();
		d1.u.val = (*bltins[i].u.f1)(d1.u.val);
		break;
	case 2:
		d2 = pop();
		d1 = pop();
		d1.u.val = (*bltins[i].u.f2)(d1.u.val, d2.u.val);
		break;
	}
	d1.u.val = errcheck(d1.u.val, bltins[i].s);
	push(d1);
}
