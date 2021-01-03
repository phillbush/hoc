%{
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hoc.h"
#include "code.h"
#include "error.h"

#define argcode(a) code((Inst){.type = NARG, .u.narg = (a)})
#define valcode(v) code((Inst){.type = VAL, .u.val = (v)})
#define oprcode(o) code((Inst){.type = OPR, .u.opr = (o)})
#define symcode(s) code((Inst){.type = SYM, .u.sym = (s)})
#define fill2(x, a, b) \
	(x)[1].type = (x)[2].type = IP, \
	(x)[1].u.ip = (a), \
	(x)[2].u.ip = (b)
#define fill3(x, a, b, c) \
	(x)[1].type = (x)[2].type = (x)[3].type = IP, \
	(x)[1].u.ip = (a), \
	(x)[2].u.ip = (b), \
	(x)[3].u.ip = (c)
#define fill4(x, a, b, c, d) \
	(x)[1].type = (x)[2].type = (x)[3].type = (x)[4].type = IP, \
	(x)[1].u.ip = (a), \
	(x)[2].u.ip = (b), \
	(x)[3].u.ip = (c), \
	(x)[4].u.ip = (d)

int yylex(void);
%}

%union {
	int narg;
	Symbol *sym;
	double val;
	Inst *inst;
}

%token <val>  NUMBER PREVIOUS
%token <sym>  VAR BLTIN UNDEF PRINT WHILE IF ELSE FOR BREAK CONTINUE
%type  <narg> args arglist
%type  <inst> expr exprlist stmt stmtlist asgn
%type  <inst> and or while if cond forcond forloop begin end
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
	| list exprlist term    { oprcode(print); oprcode(NULL); return 1; }
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

stmt:
	  '{' stmtlist '}'                      { $$ = $2; }
	| BREAK                                 { looponly($1->name); oprcode(breakcode); }
	| CONTINUE                              { looponly($1->name); oprcode(continuecode); }
	| exprlist                              { oprcode(oprpop); }
	| PRINT exprlist                        { oprcode(prexpr); $$ = $2; }
	| if cond stmt end                      { fill3($1, $3, NULL, $4); }
	| if cond stmt end ELSE stmt end        { fill3($1, $3, $6, $7); }
	| while cond stmt end                   { fill2($1, $3, $4); inloop = 0; }
	| forloop '(' forcond ';' forcond ';' forcond ')' stmt end { fill4($1, $5, $7, $9, $10); inloop = 0; }
	// | ';'           { $$ = oprcode(NULL); }         /* null statement */
	;

exprlist:
	  expr
	| exprlist ',' expr

expr:
	  NUMBER                        { $$ = oprcode(constpush); valcode($1); }
	| PREVIOUS                      { $$ = oprcode(constpush); valcode(prev); }
	| VAR                           { $$ = oprcode(sympush); symcode($1); oprcode(eval); }
	| expr '+' expr                 { oprcode(add); }
	| expr '-' expr                 { oprcode(sub); }
	| expr '*' expr                 { oprcode(mul); }
	| expr '/' expr                 { oprcode(divd); }
	| expr '%' expr                 { oprcode(mod); }
	| expr '^' expr                 { oprcode(power); }
	| expr GT expr                  { oprcode(gt); }
	| expr GE expr                  { oprcode(ge); }
	| expr LT expr                  { oprcode(lt); }
	| expr LE expr                  { oprcode(le); }
	| expr EQ expr                  { oprcode(eq); }
	| expr NE expr                  { oprcode(ne); }
	| NOT expr                      { $$ = $2; oprcode(not); }
	| INC VAR                       { $$ = oprcode(preinc); symcode($2); }
	| DEC VAR                       { $$ = oprcode(predec); symcode($2); }
	| VAR INC                       { $$ = oprcode(postinc); symcode($1); }
	| VAR DEC                       { $$ = oprcode(postdec); symcode($1); }
	| expr and expr end             { fill2($2, $3, $4); }
	| expr or expr end              { fill2($2, $3, $4); }
	| '-' expr %prec UNARYSIGN      { $$ = $2; oprcode(negate); }
	| '+' expr %prec UNARYSIGN      { $$ = $2; }
	| '(' exprlist ')'              { $$ = $2; }
	| BLTIN begin '(' arglist ')'   { $$ = $2; oprcode(bltin); symcode($1); argcode($4); }
	| asgn
	;

args:
	  expr                  { $$ = 1;  }
	| args ',' expr         { $$ = $1 + 1; }
	;

arglist:
	  /* nothing */         { $$ = 0; }
	| args
	;

forcond:
	  /* nothing */ { oprcode(NULL); }
	| exprlist      { oprcode(NULL); }
	;

cond:
	  '(' exprlist ')'  { oprcode(NULL); $$ = $2; }
	;

if:
	  IF    { $$ = oprcode(ifcode); oprcode(NULL); oprcode(NULL); oprcode(NULL); }
	;

while:
	  WHILE { $$ = oprcode(whilecode); oprcode(NULL); oprcode(NULL); inloop = 1; }
	;

forloop:
	  FOR   { $$ = oprcode(forcode); oprcode(NULL); oprcode(NULL); oprcode(NULL); oprcode(NULL); inloop = 1; }
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

and:
	  AND   { $$ = oprcode(and); oprcode(NULL); oprcode(NULL); }
	;

or:
	  OR    { $$ = oprcode(or); oprcode(NULL); oprcode(NULL); }
	;

begin:
	  /* nothing */         { $$ = progp; }
	;

end:
	  /* nothing */         { oprcode(NULL); $$ = progp; }
	;

%%

static int inloop = 0;

/* error if using break/continue out of loop */
static void
looponly(const char *s)
{
	if (!inloop)
		yyerror("%s: used outside loop", s);
}
