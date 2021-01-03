extern Inst prog[];
extern Inst *progp;
extern double prev;

void debug(void);

Inst *code(Inst inst);

void execute(Inst *);

void init(void);
void initcode(void);

/* instruction operation functions */
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
void bltin(void);
