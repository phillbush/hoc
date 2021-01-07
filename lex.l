D       [0-9]
E       ([Ee][+-]?{D}*)
L       [a-zA-Z_]
A       [a-zA-Z0-9_]
ES      (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))

%{
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "hoc.h"
#include "code.h"
#include "error.h"
#include "gramm.h"

static int makenum(char *yytext);
static int makesym(char *yytext);
static int makestr(char *yytext);
%}

%option nounput
%option noinput
%option yylineno

%%

[ \t]                           { ; }
({D}+\.?|{D}*\.{D}+){E}?        { return makenum(yytext); }
{L}{A}*                         { return makesym(yytext); }
\"([^"\\\n]|{ES})*\"            { return makestr(yytext); }
\.                              { return PREVIOUS; }
\+\+                            { return INC; }
--                              { return DEC; }
\+=                             { return ADDEQ; }
-=                              { return SUBEQ; }
\*=                             { return MULEQ; }
\/=                             { return DIVEQ; }
%=                              { return MODEQ; }
>=                              { return GE; }
>                               { return GT; }
\<=                             { return LE; }
\<                              { return LT; }
==                              { return EQ; }
!=                              { return NE; }
\|\|                            { return OR; }
&&                              { return AND; }
!                               { return NOT; }
#.*                             { return '\n'; }
\n                              { return '\n'; }
.                               { return *yytext; }

%%

static int
makenum(char *yytext)
{
	yylval.val = atof(yytext);
	return NUMBER;
}

static int
makesym(char *yytext)
{
	Symbol *sym;

	/* if name isn't found, it is installed with the type UNDEF */
	if ((sym = lookup(NULL, yytext)) == NULL)
		sym = install(NULL, yytext, UNDEF);

	yylval.sym = sym;

	/*
	 * Even if the name is installed as UNDEF,  the returned type is
	 * a VAR; because the test for whether a variable is defined has
	 * to be in the grammar, not in the lexical analyzer.     When a
	 * VAR is recognized lexically, its context is yet not known; we
	 * don't want a complaint that `x` is undefined when the context
	 * is a legal one  (such as the left side of an assignment, like
	 * `x=1`).
	 */
	return sym->type == UNDEF ? VAR : sym->type;
}

static int
makestr(char *yytext)
{
	static char indextab[] = "'\"\\abfnrtv";
	static char transtab[] = "''\"\"\\\\a\ab\bf\fn\nr\rt\tv\v";
	char *buf, *s, *t, *ind;
	ssize_t len;

	len = strlen(yytext);
	if ((buf = malloc(len)) == NULL)        /* we don't need + 1 for yytext has quotation marks */
		yyerror("out of memory");
	for (s = buf, t = yytext + 1; t - yytext + 1 < len; s++, t++) {
		if (*t != '\\') {
			*s = *t;
		} else {
			t++;
			if (strchr(indextab, *t) && (ind = strchr(transtab, *t))) {
				*s = *(ind + 1);
			} else {
				*s = *t;
			}
		}
	}
	*s = '\0';
	yylval.str = addstr(buf, 0);
	return STRING;
}
