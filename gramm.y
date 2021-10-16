%{
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hoc.h"
#include "code.h"
#include "error.h"

#define ipcode(i)  code((Inst){.type = IP, .u.ip = (i)})
#define valcode(v) code((Inst){.type = VAL, .u.val = (v)})
#define argcode(a) code((Inst){.type = NARG, .u.narg = (a)})
#define strcode(s) code((Inst){.type = STR, .u.str = (s)})
#define oprcode(o) code((Inst){.type = OPR, .u.opr = (o)})
#define namecode(n) code((Inst){.type = NAME, .u.name = (n)})
#define fill1(x, a) \
	N1((x))->type = IP, \
	N1((x))->u.ip = (a)
#define fill2(x, a, b) \
	N1((x))->type = N2((x))->type = IP, \
	N1((x))->u.ip = (a), \
	N2((x))->u.ip = (b)
#define fill3(x, a, b, c) \
	N1((x))->type = N2((x))->type = N3((x))->type = IP, \
	N1((x))->u.ip = (a), \
	N2((x))->u.ip = (b), \
	N3((x))->u.ip = (c)
#define fill4(x, a, b, c, d) \
	N1((x))->type = N2((x))->type = N3((x))->type = N4((x))->type = IP, \
	N1((x))->u.ip = (a), \
	N2((x))->u.ip = (b), \
	N3((x))->u.ip = (c), \
	N4((x))->u.ip = (d)

int yylex(void);
static void looponly(const char *);
static void defnonly(void);

static int indef;
static size_t inloop;
%}

%union {
	String *str;
	Name *name;
	Inst *inst;
	double val;
	int narg;
}

%token <str>  STRING
%token <val>  NUMBER PREVIOUS
%token <name> VAR BLTIN UNDEF
%token <name> PRINT PRINTF READ GETLINE
%token <name> WHILE DO IF ELSE FOR BREAK CONTINUE
%token <name> FUNC PROC FUNCTION PROCEDURE RETURN
%type  <name> params paramlist
%type  <narg> args arglist
%type  <inst> expr exprlist stmt stmtlist stmtnl asgn
%type  <inst> and or do while if cond forcond forloop begin end
%type  <name> procname
%left  ','
%right '=' ADDEQ SUBEQ MULEQ DIVEQ MODEQ
%left  OR
%left  AND
%left  EQ NE
%left  GT GE LT LE
%left  '+' '-'
%left  '*' '/' '%'
%right UNARYSIGN NOT INC DEC
%right '^'
%right '$'

%%

list:
	  /* nothing */         { indef = inloop = 0; }
	| list term
	| list defn term        { oprcode(NULL); return 1; }
	| list stmt term        { oprcode(NULL); return 1; }
	| list asgn term        { oprcode(oprpop); oprcode(NULL); return 1; }
	| list exprlist term    { oprcode(println); oprcode(NULL); return 1; }
	| list error term       { yyerrok; }
	;

asgn:
	  VAR '=' expr          { $$ = $3; oprcode(assign); namecode($1); }
	| VAR ADDEQ expr        { $$ = $3; oprcode(addeq); namecode($1); }
	| VAR SUBEQ expr        { $$ = $3; oprcode(subeq); namecode($1); }
	| VAR MULEQ expr        { $$ = $3; oprcode(muleq); namecode($1); }
	| VAR DIVEQ expr        { $$ = $3; oprcode(diveq); namecode($1); }
	| VAR MODEQ expr        { $$ = $3; oprcode(modeq); namecode($1); }
	| INC VAR               { $$ = oprcode(preinc); namecode($2); }
	| DEC VAR               { $$ = oprcode(predec); namecode($2); }
	| VAR INC               { $$ = oprcode(postinc); namecode($1); }
	| VAR DEC               { $$ = oprcode(postdec); namecode($1); }
	;

/* used to break line after if, else, etc */
stmtnl:
	  stmt
	| '\n' stmtnl           { $$ = $2; }
	;

stmt:
	  '{' stmtlist '}'                      { $$ = $2; }
	| BREAK                                 { looponly($1->s); oprcode(breakcode); }
	| CONTINUE                              { looponly($1->s); oprcode(continuecode); }
	| RETURN                                { defnonly(); oprcode(procret); }
	| RETURN expr                           { $$ = $2; defnonly(); oprcode(funcret); }
	| PROCEDURE begin '(' arglist ')'       { $$ = $2; oprcode(call); namecode($1); argcode($4); }
	| PRINT begin arglist                   { $$ = $2; oprcode(_print); argcode($3); }
	| PRINTF begin arglist                  { $$ = $2; oprcode(_printf); argcode($3); }
	| exprlist                              { oprcode(oprpop); }
	| if cond stmtnl end                    { fill3($1, $3, NULL, $4); }
	| if cond stmtnl end ELSE stmtnl end    { fill3($1, $3, $6, $7); }
	| while cond stmtnl end                 { fill2($1, $3, $4); inloop--; }
	| do stmtnl WHILE cond end              { fill2($1, $4, $5); inloop--; }
	| forloop '(' forcond ';' forcond ';' forcond ')' stmtnl end { fill4($1, $5, $7, $9, $10); inloop--; }
	// | ';'           { $$ = oprcode(NULL); }         /* null statement */
	;

exprlist:
	  expr
	| exprlist ',' expr

expr:
	  NUMBER                                { $$ = oprcode(constpush); valcode($1); }
	| STRING                                { $$ = oprcode(strpush); if (indef) movstr($1); strcode($1); }
	| PREVIOUS                              { $$ = oprcode(prevpush); }
	| VAR                                   { $$ = oprcode(eval); namecode($1); }
	| READ VAR                              { oprcode(readnum); namecode($2); }
	| GETLINE VAR                           { oprcode(readline); namecode($2); }
	| FUNCTION begin '(' arglist ')'        { $$ = $2; oprcode(call); namecode($1); argcode($4); }
	| '$' expr                              { $$ = $2; oprcode(cmdarg); }
	| expr '+' expr                         { oprcode(add); }
	| expr '-' expr                         { oprcode(sub); }
	| expr '*' expr                         { oprcode(mul); }
	| expr '/' expr                         { oprcode(divd); }
	| expr '%' expr                         { oprcode(mod); }
	| expr '^' expr                         { oprcode(power); }
	| expr GT expr                          { oprcode(gt); }
	| expr GE expr                          { oprcode(ge); }
	| expr LT expr                          { oprcode(lt); }
	| expr LE expr                          { oprcode(le); }
	| expr EQ expr                          { oprcode(eq); }
	| expr NE expr                          { oprcode(ne); }
	| NOT expr                              { $$ = $2; oprcode(not); }
	| expr and expr end                     { fill2($2, $3, $4); }
	| expr or expr end                      { fill2($2, $3, $4); }
	| '-' expr %prec UNARYSIGN              { $$ = $2; oprcode(negate); }
	| '+' expr %prec UNARYSIGN              { $$ = $2; }
	| '(' exprlist ')'                      { $$ = $2; }
	| BLTIN begin '(' arglist ')'           { $$ = $2; oprcode(bltin); namecode($1); argcode($4); }
	| asgn
	;

defn:
	  FUNC procname                 { indef = 1; verifydef($2, FUNCTION); }
	  '(' paramlist ')' stmtnl      { oprcode(procret); define($2, $5); indef = 0; }
	| PROC procname                 { indef = 1; verifydef($2, PROCEDURE); }
	  '(' paramlist ')' stmtnl      { oprcode(procret); define($2, $5); indef = 0; }
	;

procname:
	  VAR
	;

args:
	  expr                  { $$ = 1; }
	| args ',' expr         { $$ = $1 + 1; }
	;

arglist:
	  /* nothing */         { $$ = 0; }
	| args
	;

/*
 * we need get params in the reverse order, for that's the order in which
 * we pop() arguments in call()
 */
params:
	  VAR                   { $$ = installlocalname($1->s, NULL); }
	| params ',' VAR        { $$ = installlocalname($3->s, $1); }
	;

paramlist:
	  /* nothing */         { $$ = NULL; }
	| params
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

do:
	  DO    { $$ = oprcode(docode); oprcode(NULL); oprcode(NULL); inloop++; }
	;

while:
	  WHILE { $$ = oprcode(whilecode); oprcode(NULL); oprcode(NULL); inloop++; }
	;

forloop:
	  FOR   { $$ = oprcode(forcode); oprcode(NULL); oprcode(NULL); oprcode(NULL); oprcode(NULL); inloop++; }
	;

stmtlist:
	  /* nothing */ { $$ = getprogp(); }
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
	  /* nothing */         { $$ = getprogp(); }
	;

end:
	  /* nothing */         { oprcode(NULL); $$ = getprogp(); }
	;

%%

/* error if using break/continue out of loop */
static void
looponly(const char *s)
{
	if (!inloop)
		yyerror("%s used outside loop", s);
}

/* error if using return out of function definition */
static void
defnonly(void)
{
	if (!indef)
		yyerror("return used outside definition");
}
