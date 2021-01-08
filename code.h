#ifndef DEBUG
#define DEBUG 0
#endif

/* macros */
#define N1(p) ((p)->next)
#define N2(p) ((p)->next->next)
#define N3(p) ((p)->next->next->next)
#define N4(p) ((p)->next->next->next->next)

/* routines called by main.o */
void init(void);
void prepare(void);
void cleanup(void);
void debug(void);
void execute(Inst *);

/* routines called by lex.o */
Name *lookupname(const char *s);
Name *installglobalname(const char *s, int t);

/* routines called by gramm.o */
String *addstr(char *, int);
Name *installlocalname(const char *s, Name *nametab);
Inst *code(Inst inst);
Inst *getprogp(void);
void verifydef(Name *, int);
void define(Name *, Name *);
void movstr(String *str);

/* instruction operation routines */
void oprpop(void);
void eval(void);
void add(void);
void sub(void);
void mul(void);
void mod(void);
void divd(void);
void negate(void);
void power(void);
void assign(void);
void addeq(void);
void subeq(void);
void muleq(void);
void diveq(void);
void modeq(void);
void preinc(void);
void predec(void);
void postinc(void);
void postdec(void);
void constpush(void);
void prevpush(void);
void strpush(void);
void println(void);
void _print(void);
void _printf(void);
void _sprintf(void);
void readnum(void);
void readline(void);
void gt(void);
void ge(void);
void lt(void);
void le(void);
void eq(void);
void ne(void);
void and(void);
void or(void);
void not(void);
void ifcode(void);
void whilecode(void);
void forcode(void);
void breakcode(void);
void continuecode(void);
void bltin(void);
void call(void);
void procret(void);
void funcret(void);
