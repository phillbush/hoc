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

	if ((sym = lookup(yytext)) == NULL)
		sym = install(yytext, UNDEF, 0.0, NULL);
	yylval.sym = sym;
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
