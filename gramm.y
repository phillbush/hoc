%{
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hoc.h"
#include "code.h"
#include "args.h"
#include "symbol.h"
#include "error.h"

#define funcode(f) code((Inst){.type = FUN, .u.fun = (f)})
#define valcode(v) code((Inst){.type = VAL, .u.val = (v)})
#define oprcode(o) code((Inst){.type = OPR, .u.opr = (o)})
#define symcode(s) code((Inst){.type = SYM, .u.sym = (s)})

int yylex(void);
%}

%union {
	Arg *arg;
	Symbol *sym;
	double val;
	Inst *inst;
}

%token <val>  NUMBER PREVIOUS
%token <sym>  VAR BLTIN UNDEF PRINT WHILE IF ELSE FOR
%type  <inst> stmt asgn expr stmtlist cond while if end args arglist and
%type  <inst> or forcond forloop
%left  ','
%right '=' ADDEQ SUBEQ MULEQ DIVEQ MODEQ
%left  OR
%left  AND
%left  EQ NE
%left  GT GE LT LE
%left  '+' '-'
%left  '*' '/' '%'
%left  UNARYSIGN NOT INC DEC
%right '^'

%%

list:
	  /* nothing */
	| list term
	| list stmt term        { oprcode(NULL); return 1; }
	| list asgn term        { oprcode(oprpop); oprcode(NULL); return 1; }
	| list expr term        { oprcode(print); oprcode(NULL); return 1; }
	| list error term       { yyerrok; }
	;

asgn:
	  VAR '=' expr          { $$ = $3; oprcode(sympush); symcode($1); oprcode(assign); }
	| VAR ADDEQ expr        { $$ = $3; oprcode(sympush); symcode($1); oprcode(addeq); }
	| VAR SUBEQ expr        { $$ = $3; oprcode(sympush); symcode($1); oprcode(subeq); }
	| VAR MULEQ expr        { $$ = $3; oprcode(sympush); symcode($1); oprcode(muleq); }
	| VAR DIVEQ expr        { $$ = $3; oprcode(sympush); symcode($1); oprcode(diveq); }
	| VAR MODEQ expr        { $$ = $3; oprcode(sympush); symcode($1); oprcode(modeq); }
	;

args:
	  expr                  { $$ = oprcode(argalloc);  }
	| args ',' expr         { $$ = oprcode(argadd); }
	;

arglist:
	  /* nothing */         { $$ = oprcode(argnull); }
	| args;
	;

stmt:
	  expr          { oprcode(oprpop); }
	| PRINT expr    { oprcode(prexpr); $$ = $2; }
	| forloop '(' forcond ';' forcond ';' forcond ')' stmt end {
		($1)[1].type = ($1)[2].type = ($1)[3].type = ($1)[4].type = IP;
		($1)[1].u.ip = $5;      /* condition */
		($1)[2].u.ip = $7;      /* post loop */
		($1)[3].u.ip = $9;      /* body of loop */
		($1)[4].u.ip = $10; }   /* end, if cond fails */
	| while cond stmt end {
		($1)[1].type = ($1)[2].type = IP;
		($1)[1].u.ip = $3;     /* body of loop */
		($1)[2].u.ip = $4; }   /* end, if cond fails */
	| if cond stmt end {
		($1)[1].type = ($1)[2].type = ($1)[3].type = IP;
		($1)[1].u.ip = $3;     /* thenpart */
		($1)[2].u.ip = NULL;   /* elsepart */
		($1)[3].u.ip = $4; }   /* end, if cond fails */
	| if cond stmt end ELSE stmt end {
		($1)[1].type = ($1)[2].type = ($1)[3].type = IP;
		($1)[1].u.ip = $3;     /* thenpart */
		($1)[2].u.ip = $6;     /* elsepart */
		($1)[3].u.ip = $7; }   /* end, if cond fails */
	| '{' stmtlist '}'      { $$ = $2; }
	// | ';'           { $$ = oprcode(NULL); }         /* null statement */
	;

forcond:
	  /* nothing */ { oprcode(NULL); }
	| expr          { oprcode(NULL); }
	;

cond:
	  '(' expr ')'  { oprcode(NULL); $$ = $2; }
	;

if:
	  IF    { $$ = oprcode(ifcode); oprcode(NULL); oprcode(NULL); oprcode(NULL); }
	;

while:
	  WHILE { $$ = oprcode(whilecode); oprcode(NULL); oprcode(NULL); }
	;

forloop:
	  FOR   { $$ = oprcode(forcode); oprcode(NULL); oprcode(NULL); oprcode(NULL); oprcode(NULL); }
	;

stmtlist:
	  /* nothing */ { $$ = progp; }
	| stmtlist term
	| stmtlist stmt
	;

term:
	  ';'
	| '\n'
	;

expr:
	  NUMBER        { $$ = oprcode(constpush); valcode($1); }
	| PREVIOUS      { $$ = oprcode(constpush); valcode(prev); }
	| VAR           { $$ = oprcode(sympush); symcode($1); oprcode(eval); }
	| expr '+' expr { oprcode(add); }
	| expr '-' expr { oprcode(sub); }
	| expr '*' expr { oprcode(mul); }
	| expr '/' expr { oprcode(divd); }
	| expr '%' expr { oprcode(mod); }
	| expr '^' expr { oprcode(power); }
	| expr GT expr  { oprcode(gt); }
	| expr GE expr  { oprcode(ge); }
	| expr LT expr  { oprcode(lt); }
	| expr LE expr  { oprcode(le); }
	| expr EQ expr  { oprcode(eq); }
	| expr NE expr  { oprcode(ne); }
	| NOT expr      { $$ = $2; oprcode(not); }
	| INC VAR       { $$ = oprcode(preinc); symcode($2); }
	| DEC VAR       { $$ = oprcode(predec); symcode($2); }
	| VAR INC       { $$ = oprcode(postinc); symcode($1); }
	| VAR DEC       { $$ = oprcode(postdec); symcode($1); }
	| expr and expr end     {
		($2)[1].type = ($2)[2].type = IP;
		($2)[1].u.ip = $3;              /* right expression */
		($2)[2].u.ip = $4; }            /* end */
	| expr or expr end      {
		($2)[1].type = ($2)[2].type = IP;
		($2)[1].u.ip = $3;              /* right expression */
		($2)[2].u.ip = $4; }            /* end */
	| BLTIN '(' arglist ')'         { $$ = $3; oprcode(bltin); funcode($1->u.fun); }
	| '-' expr %prec UNARYSIGN      { $$ = $2; oprcode(negate); }
	| '+' expr %prec UNARYSIGN      { $$ = $2; }
	| '(' expr ')'                  { $$ = $2; }
	| asgn
	;

and:
	  AND                   { $$ = oprcode(and); oprcode(NULL); oprcode(NULL); }
	;

or:
	  OR                    { $$ = oprcode(or); oprcode(NULL); oprcode(NULL); }
	;

end:
	  /* nothing */         { oprcode(NULL); $$ = progp; }
	;

%%
