%{
#include <stddef.h>
#include "hoc.h"
#include "symbol.h"
#include "y.tab.h"
%}

%option nounput
%option noinput
%option yylineno

%%

[ \t] {
	;
}

([0-9]+\.?|[0-9]*\.[0-9]+)([eE][-+]?[0-9]*)? {
	sscanf(yytext, "%lf", &yylval.val);
	return NUMBER;
}

\.  {
	return PREVIOUS;
}

[a-zA-Z_][a-zA-Z0-9_]* {
	Symbol *sym;

	/* if name isn't found, it is installed with the type UNDEF */
	if ((sym = lookup(yytext)) == NULL)
		sym = install(yytext, UNDEF, 0.0, NULL);

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

\+\+ {
	return INC;
}

-- {
	return DEC;
}

\+= {
	return ADDEQ;
}

-= {
	return SUBEQ;
}

\*= {
	return MULEQ;
}

\/= {
	return DIVEQ;
}

%= {
	return MODEQ;
}

>= {
	return GE;
}

> {
	return GT;
}

\<= {
	return LE;
}

\< {
	return LT;
}

== {
	return EQ;
}

!= {
	return NE;
}

\|\| {
	return OR;
}

&& {
	return AND;
}

! {
	return NOT;
}

(#.*)?\n {
	return '\n';
}

. {
	return *yytext;
}
