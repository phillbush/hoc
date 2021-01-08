#include <ctype.h>
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
#include "gramm.h"

/* function declaration, needed for bltins[] */
static double Random(void);
static double Integer(double);

/* table of keywords */
static struct {
	char *s;
	int v;
} keywords[] = {
	{"func",        FUNC},
	{"proc",        PROC},
	{"print",       PRINT},
	{"printf",      PRINTF},
	{"read",        READ},
	{"getline",     GETLINE},
	{"if",          IF},
	{"else",        ELSE},
	{"while",       WHILE},
	{"for",         FOR},
	{"break",       BREAK},
	{"continue",    CONTINUE},
	{"return",      RETURN},
	{NULL,          0}
};

/* table of operations */
static struct {
	char *s;
	void (*f)(void);
} oprs[] = {
	{"oprpop",       oprpop},
	{"eval",         eval},
	{"add",          add},
	{"sub",          sub},
	{"mul",          mul},
	{"mod",          mod},
	{"divd",         divd},
	{"negate",       negate},
	{"power",        power},
	{"assign",       assign},
	{"addeq",        addeq},
	{"subeq",        subeq},
	{"muleq",        muleq},
	{"diveq",        diveq},
	{"modeq",        modeq},
	{"preinc",       preinc},
	{"predec",       predec},
	{"postinc",      postinc},
	{"postdec",      postdec},
	{"constpush",    constpush},
	{"prevpush",     prevpush},
	{"strpush",      strpush},
	{"println",      println},
	{"print",        _print},
	{"printf",       _printf},
	{"sprintf",      _sprintf},
	{"readnum",      readnum},
	{"readline",     readline},
	{"gt",           gt},
	{"ge",           ge},
	{"lt",           lt},
	{"le",           le},
	{"eq",           eq},
	{"ne",           ne},
	{"and",          and},
	{"or",           or},
	{"not",          not},
	{"ifcode",       ifcode},
	{"whilecode",    whilecode},
	{"forcode",      forcode},
	{"breakcode",    breakcode},
	{"continuecode", continuecode},
	{"bltin",        bltin},
	{"call",         call},
	{"procret",      procret},
	{"funcret",      funcret},
	{NULL,           NULL}
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
	{"sprintf", -2, .u.d = 0.0},    /* special bltin function, must be first */
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
	Inst *base;
	Inst *pc;
} prog = {NULL, NULL, NULL, NULL, NULL};

/* the frame stack */
static struct {
	Frame *head;    /* beginning of frame stack */
	Frame *tail;    /* current frame */
	Frame *curr;    /* current frame */
	Frame *next;    /* next available frame */
} frame = {NULL, NULL, NULL, NULL};

/* the string list */
static String *autostrings = NULL;      /* strings freed automatically after execution */
static String *finalstrings = NULL;     /* strings that should be manually freed */

/* the symbol table */
static Symbol *global = NULL;           /* global symbol table */
static Symbol *currsymtab;              /* current symbol table */

/* the name table (for keywords and variable names) */
static Name *nametab = NULL;

/* flags */
static int breaking, continuing, returning;

/* previously printed value */
static Datum prev = {.next = NULL, .isstr = 0, .u.val = 0.0};

/* check return from malloc */
static void *
emalloc(size_t n)
{
	void *p;

	if ((p = malloc(n)) == NULL)
		yyerror("out of memory");
	return p;
}

/* free string list */
static void
freestrings(String **strings)
{
	String *tmp, *p;

	p = *strings;
	while (p) {
		tmp = p;
		p = p->next;
		if (DEBUG)
			fprintf(stderr, "FREED STRING: %s\n", tmp->s);
		free(tmp->s);
		free(tmp);
	}
	*strings = NULL;
}

/* free contents of stack */
static void
freestack(void)
{
	Datum *tmp, *p = stack;

	while(p) {
		tmp = p;
		p = p->next;
		if (DEBUG)
			fprintf(stderr, "FREED STACK ENTRY (THIS SHOULD NOT OCCUR)\n");
		free(tmp);
	}
	stack = NULL;
}

/* free symbol table */
static void
freesymtab(Symbol **symtab)
{
	Symbol *tmp, *p;

	p = *symtab;
	while (p) {
		tmp = p;
		p = p->next;
		if (DEBUG)
			fprintf(stderr, "FREED SYMBOL: %s\n", tmp->name);
		free(tmp);
	}
	*symtab = NULL;
}

/* free name table */
static void
freenametab(Name **nametab)
{
	Name *tmp, *p;

	p = *nametab;
	while (p) {
		tmp = p;
		p = p->next;
		if (DEBUG)
			fprintf(stderr, "FREED NAME: %s\n", tmp->s);
		if (tmp->type == FUNCTION || tmp->type == PROCEDURE)
			freenametab(&(tmp->u.fun->params));
		free(tmp->s);
		free(tmp);
	}
	*nametab = NULL;
}

/* check return from strdup */
static char *
estrdup(const char *s)
{
	char *p;

	if ((p = strdup(s)) == NULL)
		yyerror("out of memory");
	return p;
}

/* allocate symbol */
static Symbol *
eallocsym(char *s)
{
	Symbol *sym;

	sym = emalloc(sizeof *sym);
	sym->name = s;
	sym->isstr = 0;
	sym->u.val = 0.0;
	return sym;
}

/* get name as instruction argument from prog.pc; and increment pc */
static Name *
getnamearg(void)
{
	Name *name;

	name = prog.pc->u.name;
	prog.pc = prog.pc->next;
	return name;
}

/* get integer as instruction argument from prog.pc; and increment pc */
static int
getintarg(void)
{
	int n;

	n = prog.pc->u.narg;
	prog.pc = prog.pc->next;
	return n;
}

/* get floating point as instruction argument from prog.pc; and increment pc */
static double
getvalarg(void)
{
	double v;

	v = prog.pc->u.val;
	prog.pc = prog.pc->next;
	return v;
}

/* get string as instruction argument from prog.pc; and increment pc */
static String *
getstrarg(void)
{
	String *str;

	str = prog.pc->u.str;
	prog.pc = prog.pc->next;
	return str;
}

/* find s in symtab */
static Symbol *
lookupsym(Symbol *symtab, const char *s)
{
	Symbol *sym;

	for (sym = symtab; sym; sym = sym->next)
		if (strcmp(sym->name, s) == 0)
			return sym;
	return NULL;
}

/* install s into global symbol table */
static Symbol *
installglobalsym(char *s)
{
	Symbol *sym;

	sym = eallocsym(s);
	sym->next = global;
	global = sym;
	return sym;
}

/* install s into local symbol table */
static Symbol *
installlocalsym(char *s, Symbol *local)
{
	Symbol *sym;

	sym = eallocsym(s);
	sym->next = local;
	return sym;
}

/* find name in global name table */
Name *
lookupname(const char *s)
{
	Name *name;

	for (name = nametab; name; name = name->next)
		if (strcmp(name->s, s) == 0)
			return name;
	return NULL;
}

/* instal name into global name table */
Name *
installglobalname(const char *s, int t)
{
	Name *name;

	name = emalloc(sizeof *name);
	name->s = estrdup(s);
	name->type = t;
	name->next = nametab;
	nametab = name;
	return name;
}

/* install name into local name table; all local names are of type VAR */
Name *
installlocalname(const char *s, Name *tab)
{
	Name *name;

	name = emalloc(sizeof *name);
	name->s = estrdup(s);
	name->type = VAR;
	name->next = tab;
	return name;
}

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
	p->count = 1;
	*list = p;
	return p;
}

/* return pointer to operation name */
static char *
oprname(void (*opr)(void))
{
	int i;

	if (opr == NULL)
		return "STOP";
	for (i = 0; oprs[i].f; i++)
		if (opr == oprs[i].f)
			return oprs[i].s;
	return "unknown";
}

/* initialize machine */
void
init(void)
{
	Name *name;
	int i;

	/* initialize program memory */
	prog.head = emalloc(sizeof *prog.head);
	prog.head->next = NULL;
	prog.base = prog.progp = prog.head;

	/* initialize frame stack */
	frame.head = emalloc(sizeof *frame.head);
	frame.head->next = NULL;
	frame.head->prev = NULL;
	frame.next = frame.head;

	/* initialize random function */
	srand(time(NULL));

	/* initialize symbol table with keyword and builtin names */
	for (i = 0; keywords[i].s; i++)
		installglobalname(keywords[i].s, keywords[i].v);
	for (i = 0; bltins[i].s; i++) {
		name = installglobalname(bltins[i].s, BLTIN);
		name->u.bltin = i;
	}
}

/* initialize for code generation */
void
prepare(void)
{
	Frame *fp;

	continuing = breaking = returning = 0;
	prog.tail = NULL;
	prog.progp = prog.base;
	currsymtab = NULL;
	for (fp = frame.head; fp && fp != frame.tail; fp = fp->next)
		if (fp->local)
			freesymtab(&(fp->local));
	frame.tail = frame.next = frame.head;
	frame.curr = NULL;
	freestrings(&autostrings);
	freestack();
}

/* clean up machine */
void
cleanup(void)
{
	freesymtab(&global);
	freestrings(&autostrings);
	freestrings(&finalstrings);
	freenametab(&nametab);
	freestack();
}

/* debug the machine */
void
debug(void)
{
	Inst *p;
	size_t n;

	for (n = 0, p = prog.base; p && p != prog.progp; n++, p = p->next) {
		fprintf(stderr, "CODE %03zu: ", n);
		switch (p->type) {
		case NARG:
			fprintf(stderr, "NARG %d", p->u.narg);
			break;
		case VAL:
			fprintf(stderr, "VAL  %.8g", p->u.val);
			break;
		case NAME:
			fprintf(stderr, "SYM  %s", p->u.name->s);
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
		prog.pc = prog.base;
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
		ip = emalloc(sizeof *ip);
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

	p = emalloc(sizeof *p);
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

	d.u.val = getvalarg();
	d.isstr = 0;
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

	d.u.str = getstrarg();
	d.isstr = 1;
	push(d);
}

/* free String from datum, String should be listed on finalstrings! */
static void
dfree(String *str)
{
	if (str->count > 1) {
		str->count--;
	} else {
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
}

/* move string from autostrings to finalstrings */
void
movstr(String *str)
{
	if (str->isfinal) {
		str->count++;
	} else {
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
		str->count = 1;
		finalstrings = str;
	}
}

/* pop numeric value from stack */
static Datum
popnum(void)
{
	Datum d;
	double v;

	d = pop();
	if (d.isstr) {
		v = atof(d.u.str->s);
		d.u.val = v;
	}
	d.isstr = 0;
	return d;
}

/* add top two elements on stack */
void
add(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val += d2.u.val;
	push(d1);
}

/* subtract top two elements on stack */
void
sub(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val -= d2.u.val;
	push(d1);
}

/* multiply top two elements on stack */
void
mul(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val *= d2.u.val;
	push(d1);
}

/* divide top two elements on stack */
void
divd(void)
{
	Datum d1, d2;

	d2 = popnum();
	if (d2.u.val == 0.0)
		yyerror("division by zero");
	d1 = popnum();
	d1.u.val /= d2.u.val;
	push(d1);
}

/* compute module of top two elements on stack */
void
mod(void)
{
	Datum d1, d2;

	d2 = popnum();
	if (d2.u.val == 0.0)
		yyerror("module by zero");
	d1 = popnum();
	d1.u.val = fmod(d1.u.val, d2.u.val);
	push(d1);
}

/* negate top element on stack */
void
negate(void)
{
	Datum d;

	d = popnum();
	d.u.val = -d.u.val;
	push(d);
}

/* compute power of top two elements on stack */
void
power(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val = pow(d1.u.val, d2.u.val);
	push(d1);
}

/* evaluate variable on stack */
void
eval(void)
{
	Symbol *sym = NULL;
	Name *name;
	Datum d;

	name = getnamearg();
	if ((sym = lookupsym(currsymtab, name->s)) == NULL)
		if ((sym = lookupsym(global, name->s)) == NULL)
			yyerror("could not find variable %s", name->s);
	d.isstr = sym->isstr;
	d.u = sym->u;
	push(d);
}

/* verify whether name is assignable and undefined */
static void
verifyassign(Name *name, int undef)
{
	if (undef && name->type == UNDEF)
		yyerror("undefined variable: %s", name->s);
	if (name->type != VAR && name->type != UNDEF)
		yyerror("assignment to non-variable: %s", name->s);
}

/* get symbol from name for assignment; and convert to number if convtonum != 0 */
static Symbol *
getassign(int convtonum)
{
	Name *name;
	Symbol *sym;
	double v;

	name = getnamearg();
	verifyassign(name, convtonum);
	if ((sym = lookupsym(currsymtab, name->s)) == NULL)
		if ((sym = lookupsym(global, name->s)) == NULL)
			sym = installglobalsym(name->s);
	name->type = VAR;
	if (convtonum && sym->isstr) {
		v = atof(sym->u.str->s);
		dfree(sym->u.str);
		sym->u.val = v;
		sym->isstr = 0;
	}
	return sym;
}

/* pre-increment variable */
void
preinc(void)
{
	Symbol *sym;
	Datum d;

	sym = getassign(1);
	d.u.val = sym->u.val += 1.0;
	d.isstr = 0;
	push(d);
}

/* pre-decrement variable */
void
predec(void)
{
	Symbol *sym;
	Datum d;

	sym = getassign(1);
	d.u.val = sym->u.val -= 1.0;
	d.isstr = 0;
	push(d);
}

/* post-increment variable */
void
postinc(void)
{
	Symbol *sym;
	Datum d;

	sym = getassign(1);
	d.u.val = sym->u.val;
	d.isstr = 0;
	sym->u.val += 1.0;
	push(d);
}

/* post-decrement variable */
void
postdec(void)
{
	Symbol *sym;
	Datum d;

	sym = getassign(1);
	d.u.val = sym->u.val;
	d.isstr = 0;
	sym->u.val -= 1.0;
	push(d);
}

/* assign top value to next value */
void
assign(void)
{
	Datum d;
	Symbol *sym;

	d = pop();
	sym = getassign(0);
	if (sym->isstr)
		dfree(sym->u.str);
	if (d.isstr)
		movstr(d.u.str);
	sym->u = d.u;
	sym->isstr = d.isstr;
	push(d);
}

/* add and assign top value to next value */
void
addeq(void)
{
	Symbol *sym;
	Datum d;

	d = popnum();
	sym = getassign(1);
	d.u.val = sym->u.val += d.u.val;
	push(d);
}

/* subtract and assign top value to next value */
void
subeq(void)
{
	Symbol *sym;
	Datum d;

	d = popnum();
	sym = getassign(1);
	d.u.val = sym->u.val -= d.u.val;
	push(d);
}

/* multiply and assign top value to next value */
void
muleq(void)
{
	Symbol *sym;
	Datum d;

	d = popnum();
	sym = getassign(1);
	d.u.val = sym->u.val *= d.u.val;
	push(d);
}

/* divide and assign top value to next value */
void
diveq(void)
{
	Symbol *sym;
	Datum d;

	d = popnum();
	sym = getassign(1);
	d.u.val = sym->u.val /= d.u.val;
	push(d);
}

/* compute module and assign top value to next value */
void
modeq(void)
{
	Symbol *sym;
	Datum d;
	double v;

	d = popnum();
	sym = getassign(1);
	v = fmod(sym->u.val, d.u.val);
	d.u.val = sym->u.val = v;
	push(d);
}

/* print content of datum */
static void
pr(Datum d)
{
	if (d.isstr)
		printf("%s", d.u.str->s);
	else
		printf("%.8g", d.u.val);
}

void
println(void)
{
	Datum d;

	d = pop();
	pr(d);
	printf("\n");
	if (prev.isstr)
		dfree(prev.u.str);
	if (d.isstr)
		movstr(d.u.str);
	prev = d;
}

/* free list got by poplist */
static void
freelist(Datum *p)
{
	Datum *tmp;

	while (p) {
		tmp = p;
		p = p->next;
		free(tmp);
	}
}

/* get narg data from stack in reverse order */
static Datum *
poplist(void)
{
	Datum *tmp = NULL, *beg = NULL;
	int narg;

	narg = getintarg();
	while (narg-- > 0) {
		if (stack == NULL) {
			freelist(beg);
			yyerror("stack underflow");
		}
		beg = stack;
		stack = stack->next;
		beg->next = tmp;
		tmp = beg;
	}
	return beg;
}

/* print list of expressions */
void
_print(void)
{
	Datum *beg, *p;

	beg = poplist();
	for (p = beg; p; p = p->next) {
		pr(*p);
		if (p->next)
			printf(" ");
		else
			printf("\n");
	}
	freelist(beg);
}

/* printf-like conversions */
static char *
format(char *s, Datum *p)
{
	char buf[BUFSIZ];
	char *fmt, *save, *t;
	int n;

	fmt = NULL;
	t = buf; 
	while (*s) {
		if (t + 1 >= buf + sizeof buf)
			goto error;
		if (*s != '%') {
			*t++ = *s++;
			continue;
		}
		if (*(s+1) == '%') {
			*t++ = '%';
			s += 2;
			continue;
		}
		save = s++;
		while (strchr("#-+ 0", *s))
			s++;
		while (isdigit(*s))
			s++;
		if (*s == '.')
			s++;
		while (isdigit(*s))
			s++;
		if ((fmt = strndup(save, s - save + 1)) == NULL)
			goto error;
		switch (*s) {
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'X':
		case 'x':
			/* int */
			if (!p || p->isstr)
				goto wrong;
			n = snprintf(t, BUFSIZ - (t - buf), fmt, (int)p->u.val);
			break;
		case 'f':
		case 'F':
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		case 'a':
		case 'A':
			/* double */
			if (!p || p->isstr)
				goto wrong;
			n = snprintf(t, BUFSIZ - (t - buf), fmt, p->u.val);
			break;
		case 'c':
			/* char */
			if (!p || !p->isstr)
				goto wrong;
			n = snprintf(t, BUFSIZ - (t - buf), fmt, *p->u.str->s);
			break;
		case 's':
			/* string */
			if (!p || !p->isstr)
				goto wrong;
			n = snprintf(t, BUFSIZ - (t - buf), fmt, p->u.str->s);
			break;
		default:
			if ((n = strlen(fmt)) < BUFSIZ - (t - buf))
				strncpy(t, fmt, strlen(fmt));
			else
				goto error;
			break;
		}
		s++;
		if (n > BUFSIZ - (t - buf) + 1)
			goto error;
		t += n;
		p = p->next;
		free(fmt);
		fmt = NULL;
	}
	*t = '\0';
	if ((s = strdup(buf)) == NULL)
		goto error;
	return s;

wrong:
	free(fmt);
	warning("wrong format");
	return NULL;

error:
	free(fmt);
	warning("out of memory");
	return NULL;
}

/* print formated list of expressions */
void
_printf(void)
{
	Datum *beg;
	char *s;

	if ((beg = poplist()) == NULL)
		goto error;
	if (!beg->isstr) {
		warning("no format supplied");
		goto error;
	}
	if ((s = format(beg->u.str->s, beg->next)) == NULL)
		goto error;
	printf("%s", s);
	free(s);
	freelist(beg);
	return;

error:
	freelist(beg);
	longjump();
}

/* push formated string onto stack */
void
_sprintf(void)
{
	String *str;
	Datum d, *beg;
	char *s;

	if ((beg = poplist()) == NULL)
		goto error;
	if (!beg->isstr) {
		warning("no format supplied");
		goto error;
	}
	if ((s = format(beg->u.str->s, beg->next)) == NULL)
		goto error;
	if ((str = addstr(s, 0)) == NULL) {
		warning("out of memory");
		goto error;
	}
	freelist(beg);
	d.isstr = 1;
	d.u.str = str;
	push(d);
	return;

error:
	freelist(beg);
	longjump();
}

/* read number into variable */
void
readnum(void)
{
	Datum d;
	Symbol *sym;
	double v;

	sym = getassign(0);
	switch (scanf("%lf", &v)) {
	case EOF:
		d.u.val = 0.0;
		break;
	case 0:
		yyerror("non-number read into %s", sym->name);
		break;
	default:
		d.u.val = 1.0;
		sym->isstr = 0;
		sym->u.val = v;
		break;
	}
	d.isstr = 0;
	push(d);
}

/* read into variable */
void
readline(void)
{
	Datum d;
	Symbol *sym;
	char buf[BUFSIZ];
	char *s;

	sym = getassign(0);
	if (fgets(buf, sizeof buf, stdin)) {
		d.u.val = 1.0;
		if ((s = strdup(buf)) == NULL)
			yyerror("out of memory");
		sym->isstr = 1;
		sym->u.str = addstr(s, 1);
	} else {
		d.u.val = 0.0;
	}
	d.isstr = 0;
	push(d);
}

void
gt(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val = (double)(d1.u.val > d2.u.val);
	push(d1);
}

void
ge(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val = (double)(d1.u.val >= d2.u.val);
	push(d1);
}

void
lt(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val = (double)(d1.u.val < d2.u.val);
	push(d1);
}

void
le(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val = (double)(d1.u.val <= d2.u.val);
	push(d1);
}

void
eq(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val = (double)(d1.u.val == d2.u.val);
	push(d1);
}

void
ne(void)
{
	Datum d1, d2;

	d2 = popnum();
	d1 = popnum();
	d1.u.val = (double)(d1.u.val != d2.u.val);
	push(d1);
}

void
not(void)
{
	Datum d;

	d = popnum();
	d.u.val = (double)(!d.u.val);
	push(d);
}

void
and(void)
{
	Datum d;
	Inst *savepc;

	savepc = prog.pc;
	d = popnum();
	if (d.u.val) {
		execute(savepc->u.ip);
		d = popnum();
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
	d = popnum();
	if (!d.u.val) {
		execute(savepc->u.ip);
		d = popnum();
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
	d = popnum();
	return d.u.val;
}

static Datum
execpop(Inst *pc)
{
	Datum d;

	if (pc && pc->u.opr) {
		execute(pc);
		d = popnum();
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
	if (!returning)
		prog.pc = N2(savepc)->u.ip;     /* next statement */
}

void
whilecode(void)
{
	Inst *savepc;

	savepc = prog.pc;
	while (cond(N2(savepc))) {
		execute(savepc->u.ip);
		if (returning) {
			break;
		}
		if (continuing) {
			continuing = 0;
			continue;
		}
		if (breaking) {
			breaking = 0;
			break;
		}
	}
	if (!returning)
		prog.pc = N1(savepc)->u.ip;
}

void
forcode(void)
{
	Inst *savepc;

	savepc = prog.pc;
	for ((void)execpop(N4(savepc)); cond(savepc->u.ip); (void)execpop(N1(savepc)->u.ip)) {
		execute(N2(savepc)->u.ip);
		if (returning) {
			break;
		}
		if (continuing) {
			continuing = 0;
			continue;
		}
		if (breaking) {
			breaking = 0;
			break;
		}
	}
	if (!returning)
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
	Name *name;
	int narg, i;

	name = getnamearg();
	i = name->u.bltin;
	if (i == 0) {   /* sprintf */
		_sprintf();
		return;
	}
	narg = getintarg();
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
		d1 = popnum();
		d1.u.val = (*bltins[i].u.f1)(d1.u.val);
		break;
	case 2:
		d2 = popnum();
		d1 = popnum();
		d1.u.val = (*bltins[i].u.f2)(d1.u.val, d2.u.val);
		break;
	}
	d1.u.val = errcheck(d1.u.val, bltins[i].s);
	d1.isstr = 0;
	push(d1);
}

/* check if function or procedure is definable */
void
verifydef(Name *name, int type)
{
	if (name->type != UNDEF)
		yyerror("assigning %s to an already used name: %d",
		        (type == FUNCTION) ? "function" : "procedure",
		        name->type);
	name->type = type;
}

/* put function or procedure in symbol table */
void
define(Name *name, Name *params)
{
	Function *fun;
	int n;

	if (DEBUG)
		debug();
	fun = emalloc(sizeof *fun);
	fun->code = prog.base;          /* start of code */
	prog.base = prog.progp;         /* next code starts here */
	fun->params = params;
	for (n = 0; params; params = params->next)
		n++;
	fun->nparams = n;
	name->u.fun = fun;
}

/* call a function */
void
call(void)
{
	Symbol *local;
	Frame *f;
	Datum d;
	Name *name, *tmp;
	int nargs;

	name = getnamearg();
	if (name->type != FUNCTION && name->type != PROCEDURE)
		yyerror("%s is not function nor procedure", name->s);
	nargs = getintarg();
	if (!frame.next->next) {
		f = emalloc(sizeof *f);
		f->next = NULL;
		frame.next->next = f;
	}
	frame.curr = f = frame.next;
	frame.tail = frame.next = frame.next->next;
	frame.next->prev = frame.curr;
	if (nargs > name->u.fun->nparams)
		yyerror("function %s called with wrong number of parameters", name->s);
	nargs = name->u.fun->nparams - nargs;
	local = NULL;
	for (tmp = name->u.fun->params; tmp; tmp = tmp->next) {
		if (nargs > 0) {
			d.u.val = 0.0;
			d.isstr = 0;
			nargs--;
		} else {
			d = pop();
		}
		local = installlocalsym(tmp->s, local);
		local->u = d.u;
		local->isstr = d.isstr;
	}
	f->name = name;
	f->retsymtab = currsymtab;
	currsymtab = f->local = local;
	f->retpc = prog.pc;
	execute(name->u.fun->code);
	returning = 0;
}

/* common return from func or proc */
static void
ret(void)
{
	currsymtab = frame.curr->retsymtab;
	prog.pc = frame.curr->retpc;
	frame.next = frame.curr;
	frame.curr = frame.curr->prev;
	returning = 1;
}

/* return from a function */
void
funcret(void)
{
	Datum d;

	if (frame.curr->name->type == PROCEDURE)
		yyerror("%s (proc) returns value", frame.curr->name->s);
	d = pop();
	ret();
	push(d);
}

/* return from a procedure */
void
procret(void)
{
	if (frame.curr->name->type == FUNCTION)
		yyerror("%s (func) returns no value", frame.curr->name->s);
	ret();
}
