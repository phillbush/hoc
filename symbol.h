/* symbol table entry */
typedef struct Symbol {
	struct Symbol *next;
	char *name;
	int type;
	double val;
} Symbol;

Symbol *lookup(const char *s);
Symbol *install(const char *s, int t, double d);
void cleansym(void);
