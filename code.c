#include <stdio.h>
#include <math.h>
#include "hoc.h"
#include "args.h"
#include "error.h"
#include "code.h"
#include "bltin.h"
#include "gramm.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#define NSTACK 256
#define NPROG  2000

/* keywords */
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
	{"argnull",   argnull},
	{"argalloc",  argalloc},
	{"argadd",    argadd},
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

static Datum stack[NSTACK];     /* the stack */
static Datum *stackp;           /* the free spot on stack */

Inst prog[NPROG];               /* the machine */
Inst *progp;                    /* next free spot for code generation */
Inst *pc;                       /* program counter during execution */

double prev = 0;

/* push d onto stack */
static void
push(Datum d)
{
	if (stackp >= &stack[NSTACK])
		yyerror("stack overflow");
	*stackp++ = d;
}

/* pop and return top element from stack */
static Datum
pop(void)
{
	if (stackp <= stack)
		yyerror("stack underflow");
	return *--stackp;
}

static void
verifyassign(Symbol *s)
{
	if (s->type != VAR && s->type != UNDEF)
		yyerror("assignment to non-variable: %s", s->name);
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

/* install one instruction or operand */
Inst *
code(Inst inst)
{
	Inst *oprogp;

	oprogp = progp;
	if (progp >= &prog[NPROG])
		yyerror("program too big");
	*progp++ = inst;
	return oprogp;
}

/* initialize for code generation */
void
initcode(void)
{
	stackp = stack;
	progp = prog;
}

/* return pointer to operation name */
char *
oprname(void (*opr)(void))
{
	size_t i;

	for (i = 0; oprs[i].f; i++)
		if (opr == oprs[i].f)
			return oprs[i].s;
	return "unknown";
}

/* return pointer to built-in name */
char *
funname(double (*fun)(Arg *))
{
	size_t i;

	for (i = 0; bltins[i].f; i++)
		if (fun == bltins[i].f)
			return bltins[i].s;
	return "unknown";
}

/* debug the machine */
void
debug(void)
{
	Inst *p;
	size_t n;

	for (n = 0, p = prog; p < progp; n++, p++) {
		fprintf(stderr, "%03zu: ", n);
		switch (p->type) {
		case VAL:
			fprintf(stderr, "VAL %.8g", p->u.val);
			break;
		case SYM:
			fprintf(stderr, "SYM %s", p->u.sym->name);
			break;
		case OPR:
			fprintf(stderr, "OPR %s", oprname(p->u.opr));
			break;
		case FUN:
			fprintf(stderr, "FUN %s", funname(p->u.fun));
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
execute(Inst *p)
{
	Inst *opc;

	pc = p;
	while (pc->u.opr) {
		opc = pc++;
		opc->u.opr();
	}
}

/* pop and return top element from stack */
void
oprpop(void)
{
	pop();
}

/* push null argument list onto stack */
void
argnull(void)
{
	Datum d;

	d.arg = NULL;
	push(d);
}

/* allocate new argument list onto stack */
void
argalloc(void)
{
	Datum d;

	d = pop();
	d.arg = eallocargs(d.val);
	push(d);
}

/* allocate new argument list onto stack */
void
argadd(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.arg = addarg(d1.arg, d2.val);
	push(d1);
}

/* push constant onto stack */
void
constpush(void)
{
	Datum d;

	d.val = (*pc++).u.val;
	push(d);
}

/* push symbol onto stack */
void
sympush(void)
{
	Datum d;

	d.sym = (*pc++).u.sym;
	push(d);
}

/* add top two elements on stack */
void
add(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val += d2.val;
	push(d1);
}

/* subtract top two elements on stack */
void
sub(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val -= d2.val;
	push(d1);
}

/* multiply top two elements on stack */
void
mul(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val *= d2.val;
	push(d1);
}

/* divide top two elements on stack */
void
divd(void)
{
	Datum d1, d2;

	d2 = pop();
	if (d2.val == 0.0)
		yyerror("division by zero");
	d1 = pop();
	d1.val /= d2.val;
	push(d1);
}

/* compute module of top two elements on stack */
void
mod(void)
{
	Datum d1, d2;

	d2 = pop();
	if (d2.val == 0.0)
		yyerror("module by zero");
	d1 = pop();
	d1.val = fmod(d1.val, d2.val);
	push(d1);
}

/* negate top element on stack */
void
negate(void)
{
	Datum d;

	d = pop();
	d.val = -d.val;
	push(d);
}

/* compute power of top two elements on stack */
void
power(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val = pow(d1.val, d2.val);
	push(d1);
}

/* evaluate variable on stack */
void
eval(void)
{
	Datum d;

	d = pop();
	verifyeval(d.sym);
	d.val = d.sym->u.val;
	push(d);
}

/* pre-increment variable */
void
preinc(void)
{
	Datum d;

	d.sym = (*pc++).u.sym;
	verifyeval(d.sym);
	d.val = d.sym->u.val += 1.0;
	push(d);
}

/* pre-decrement variable */
void
predec(void)
{
	Datum d;

	d.sym = (*pc++).u.sym;
	verifyeval(d.sym);
	d.val = d.sym->u.val -= 1.0;
	push(d);
}

/* post-increment variable */
void
postinc(void)
{
	Datum d;
	double v;

	d.sym = (*pc++).u.sym;
	verifyeval(d.sym);
	v = d.sym->u.val;
	d.sym->u.val += 1.0;
	d.val = v;
	push(d);
}

/* post-decrement variable */
void
postdec(void)
{
	Datum d;
	double v;

	d.sym = (*pc++).u.sym;
	verifyeval(d.sym);
	v = d.sym->u.val;
	d.sym->u.val -= 1.0;
	d.val = v;
	push(d);
}

/* assign top value to next value */
void
assign(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.sym);
	d1.sym->u.val = d2.val;
	d1.sym->type = VAR;
	push(d2);
}

/* add and assign top value to next value */
void
addeq(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.sym);
	d2.val = d1.sym->u.val += d2.val;
	d1.sym->type = VAR;
	push(d2);
}

/* subtract and assign top value to next value */
void
subeq(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.sym);
	d2.val = d1.sym->u.val -= d2.val;
	d1.sym->type = VAR;
	push(d2);
}

/* multiply and assign top value to next value */
void
muleq(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.sym);
	d2.val = d1.sym->u.val *= d2.val;
	d1.sym->type = VAR;
	push(d2);
}

/* divide and assign top value to next value */
void
diveq(void)
{
	Datum d1, d2;

	d1 = pop();
	d2 = pop();
	verifyassign(d1.sym);
	d2.val = d1.sym->u.val /= d2.val;
	d1.sym->type = VAR;
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
	verifyassign(d1.sym);
	n = d1.sym->u.val;
	n %= (long)d2.val;
	d2.val = d1.sym->u.val = n;
	d1.sym->type = VAR;
	push(d2);
}

/* pop top value from stack, print it */
void
print(void)
{
	Datum d;

	d = pop();
	printf("\t%.8g\n", d.val);
	prev = d.val;
}

/* evaluate built-in on top of stack */
void
bltin(void)
{
	Datum d;
	double n;

	d = pop();
	n = (*pc++).u.fun(d.arg);
	delargs(d.arg);
	d.val = n;
	push(d);
}

void
prexpr(void)
{
	Datum d;

	d = pop();
	printf("%.8g\n", d.val);
}

void
gt(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val > d2.val);
	push(d1);
}

void
ge(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val >= d2.val);
	push(d1);
}

void
lt(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val < d2.val);
	push(d1);
}

void
le(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val <= d2.val);
	push(d1);
}

void
eq(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val == d2.val);
	push(d1);
}

void
ne(void)
{
	Datum d1, d2;

	d2 = pop();
	d1 = pop();
	d1.val = (double)(d1.val != d2.val);
	push(d1);
}

void
not(void)
{
	Datum d;

	d = pop();
	d.val = (double)(!d.val);
	push(d);
}

void
and(void)
{
	Datum d;
	Inst *savepc;

	savepc = pc;
	d = pop();
	if (d.val) {
		execute(savepc->u.ip);
		d = pop();
	}
	d.val = d.val ? 1.0 : 0.0;
	push(d);
	pc = (savepc + 1)->u.ip;
}

void
or(void)
{
	Datum d;
	Inst *savepc;

	savepc = pc;
	d = pop();
	if (!d.val) {
		execute(savepc->u.ip);
		d = pop();
	}
	d.val = d.val ? 1.0 : 0.0;
	push(d);
	pc = (savepc + 1)->u.ip;
}

void
ifcode(void)
{
	Datum d;
	Inst *savepc;

	savepc = pc;                            /* then part */
	execute(savepc + 3);
	d = pop();
	if (d.val)
		execute(savepc->u.ip);
	else if ((savepc + 1)->u.ip)            /* else part? */
		execute((savepc + 1)->u.ip);
	pc = (savepc + 2)->u.ip;                /* next statement */
}

static double
cond(Inst *pc)
{
	Datum d;

	if (pc == NULL)
		return 1.0;
	execute(pc);
	d = pop();
	return d.val;
}

static void
execpop(Inst *pc)
{
	execute(pc);
	pop();
}

void
whilecode(void)
{
	Inst *savepc;

	savepc = pc;
	while (cond(savepc + 2))
		execute(savepc->u.ip);
	pc = (savepc + 1)->u.ip;
}

void
forcode(void)
{
	Inst *savepc;

	savepc = pc;
	for (execpop(savepc + 4); cond(savepc->u.ip); execpop((savepc + 1)->u.ip))
		execute((savepc + 2)->u.ip);
	pc = (savepc + 3)->u.ip;
}
