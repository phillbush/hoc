/* string entry type */
typedef struct String {
	struct String *prev, *next;
	char *s;
	int isfinal;
	size_t count;
} String;

/* datum content type */
typedef union Value {
	struct Symbol *sym;
	struct String *str;
	struct Function *fun;
	double val;
	int bltin;
} Value;

/* symbol table entry */
typedef struct Symbol {
	struct Symbol *next;
	union Value u;
	char *name;
	int type;
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
	enum {VAL, STR, SYM, OPR, IP, NARG} type;
	union {
		struct String *str;
		struct Symbol *sym;
		struct Inst *ip;
		void (*opr)(void);
		double val;
		int narg;
	} u;
} Inst;

/* procedure/function definition type */
typedef struct Function {
	struct Inst *code;
	struct Symbol *args;
	size_t nargs;
} Function;

/* procedure/function call stack frame */
typedef struct Frame {
	struct Frame *prev, *next;
	struct Symbol *symtab;          /* local symbol table */
	struct Int *retpc;              /* where to resume after return */
} Frame;
