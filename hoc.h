/* name entry type */
typedef struct Name {
	struct Name *next;
	char *s;
	int type;
	union {
		struct Function *fun;
		int bltin;
	} u;
} Name;

/* string entry type */
typedef struct String {
	struct String *prev, *next;
	char *s;
	int isfinal;
	size_t count;
} String;

/* datum content type */
typedef union Value {
	struct String *str;
	double val;
} Value;

/* symbol table entry */
typedef struct Symbol {
	struct Symbol *next;
	union Value u;
	char *name;
	int isstr;
} Symbol;

/* interpreter stack type */
typedef struct Datum {
	struct Datum *next;
	union Value u;
	int isstr;
} Datum;

/* machine instruction type */
typedef struct Inst {
	struct Inst *next;
	enum {VAL, STR, NAME, OPR, IP, NARG} type;
	union {
		struct String *str;
		struct Name *name;
		struct Inst *ip;
		void (*opr)(void);
		double val;
		int narg;
	} u;
} Inst;

/* procedure/function definition type */
typedef struct Function {
	struct Inst *code;
	struct Name *params;
	int nparams;
} Function;

/* procedure/function call stack frame */
typedef struct Frame {
	struct Frame *prev, *next;
	struct Symbol *local;           /* local symbol table */
	struct Symbol *retsymtab;
	struct Name *name;
	struct Inst *retpc;             /* where to resume after return */
} Frame;
