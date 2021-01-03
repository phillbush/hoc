#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "symbol.h"

static Symbol *symlist = NULL;   /* symbol table: linked list */

static void *
emalloc(size_t n)
{
	void *p;

	if ((p = malloc(n)) == NULL)
		yyerror("out of memory");
	return p;
}

static char *
estrdup(const char *s)
{
	char *p;

	if ((p = strdup(s)) == NULL)
		yyerror("out of memory");
	return p;
}

/* find s in symbol table */
Symbol *
lookup(const char *s)
{
	Symbol *sym;

	for (sym = symlist; sym; sym = sym->next)
		if (strcmp(sym->name, s) == 0)
			return sym;
	return NULL;
}

/* install s in symbol table */
Symbol *
install(const char *s, int t, double d)
{
	Symbol *sym;

	sym = emalloc(sizeof *sym);
	sym->name = estrdup(s);
	sym->type = t;
	sym->val = d;
	sym->next = symlist;
	symlist = sym;
	return sym;
}

/* free symbol table */
void
cleansym(void)
{
	Symbol *sym, *tmp;

	sym = symlist;
	while (sym) {
		tmp = sym;
		sym = sym->next;
		if (tmp->name)
			free(tmp->name);
		free(tmp);
	}
}
