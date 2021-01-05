#include <err.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>

static char buf[BUFSIZ];
extern jmp_buf begin;
extern int yylineno;

/* jump to main loop */
void
longjump(void)
{
	longjmp(begin, 1);
}

/* warn an execution error */
void
warning(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof buf - 1, fmt, ap);
	warnx("line %d: %s", yylineno, buf);
	va_end(ap);
	errno = 0;
}

/* warn an execution error and jump to main loop */
void
yyerror(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof buf - 1, fmt, ap);
	warnx("line %d: %s", yylineno, buf);
	va_end(ap);
	errno = 0;
	longjmp(begin, 1);
}
