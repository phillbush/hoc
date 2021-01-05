/* string entry type */
typedef struct String {
	struct String *prev, *next;
	char *s;
	int isfinal;
} String;

/* datum content type */
typedef union Value {
	struct Symbol *sym;
	struct String *str;
	double val;
} Value;

/* symbol table entry */
typedef struct Symbol {
	struct Symbol *next;
	char *name;
	int type;
	int isstr;
	union Value u;
} Symbol;

/* interpreter stack type */
typedef struct Datum {
	struct Datum *next;
	int isstr;
	union Value u;
} Datum;

/* machine instruction type */
typedef struct Inst {
	struct Inst *next;
	enum {VAL, STR, SYM, OPR, IP, NARG} type;
	union {
		double val;
		String *str;
		struct Symbol *sym;
		void (*opr)(void);
		struct Inst *ip;
		int narg;
	} u;
} Inst;
