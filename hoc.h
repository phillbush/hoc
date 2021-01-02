#include <stddef.h>

typedef struct Arg {
	size_t narg;
	size_t max;
	double *a;
} Arg;

/* symbol table entry */
typedef struct Symbol {
	struct Symbol *next;
	char *name;
	int type;
	union {
		double val;
		double (*fun)(Arg *);
	} u;
} Symbol;

/* interpreter stack type */
typedef union Datum {
	double val;
	Symbol *sym;
	Arg *arg;
} Datum;

enum Insttype {VAL, SYM, OPR, FUN, IP};

/* machine instruction */
typedef struct Inst {
	enum Insttype type;
	union {
		double val;
		Symbol *sym;
		void (*opr)(void);
		double (*fun)(Arg *);
		struct Inst *ip;
	} u;
} Inst;
