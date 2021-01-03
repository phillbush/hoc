#include <stddef.h>

#ifndef DEBUG
#define DEBUG 0
#endif

enum Insttype {VAL, SYM, OPR, IP, NARG};

/* symbol table entry */
typedef struct Symbol {
	struct Symbol *next;
	char *name;
	int type;
	double val;
} Symbol;

/* interpreter stack type */
typedef union Datum {
	double val;
	Symbol *sym;
} Datum;

/* machine instruction */
typedef struct Inst {
	enum Insttype type;
	union {
		double val;
		Symbol *sym;
		void (*opr)(void);
		struct Inst *ip;
		int narg;
	} u;
} Inst;
