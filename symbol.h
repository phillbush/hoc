Symbol *lookup(const char *s);
Symbol *install(const char *s, int t, double d, double (*f)(Arg *));
void cleansym(void);
