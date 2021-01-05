/* string entry type */
typedef struct String {
	struct String *prev, *next;
	char *s;
} String;

/* symbol table entry */
typedef struct Symbol {
	struct Symbol *next;
	char *name;
	int type;
	int isstr;
	union {
		double val;
		String *str;
	} u;
} Symbol;

/* interpreter stack type */
typedef struct Datum {
	struct Datum *next;
	int isstr;
	union {
		Symbol *sym;
		String *str;
		double val;
	} u;
} Datum;

/* machine instruction type */
typedef struct Inst {
	struct Inst *next;
	enum {VAL, STR, SYM, OPR, IP, NARG} type;
	union {
		double val;
		String *str;
		Symbol *sym;
		void (*opr)(void);
		struct Inst *ip;
		int narg;
	} u;
} Inst;
