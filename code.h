#ifndef DEBUG
#define DEBUG 0
#endif

/* macros */
#define N1(p) ((p)->next)
#define N2(p) ((p)->next->next)
#define N3(p) ((p)->next->next->next)
#define N4(p) ((p)->next->next->next->next)

/* interpreter stack type */
typedef struct Datum {
	struct Datum *next;
	union {
		double val;
		Symbol *sym;
	} u;
} Datum;

/* machine instruction type */
typedef struct Inst {
	struct Inst *next;
	enum {VAL, SYM, OPR, IP, NARG} type;
	union {
		double val;
		Symbol *sym;
		void (*opr)(void);
		struct Inst *ip;
		int narg;
	} u;
} Inst;

/* routines called by main.o */
void init(void);
void initcode(void);
void debug(void);
void execute(Inst *);

/* routines called by gramm.o */
Inst *code(Inst inst);
Inst *getprogp(void);

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
void sympush(void);
void constpush(void);
void prevpush(void);
void print(void);
void prexpr(void);
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
