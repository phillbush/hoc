PROG = hoc
OBJS = main.o error.o symbol.o code.o gramm.o lex.o

CC = cc
LEX = lex
YACC = yacc
YFLAGS = -d
LFLAGS =
CPPFLAGS =
CFLAGS = -g -O0 -Wall -Wextra ${INCS}
LDLIBS = -lm -lfl
LDFLAGS = ${LDLIBS}

all: ${PROG}

${OBJS}:  hoc.h
code.o:   code.h error.h symbol.h gramm.h
lex.o:    code.h error.h symbol.h gramm.h
gramm.o:  code.h error.h
main.o:   code.h
symbol.o: error.h
error.o:

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -c $<

gramm.h: gramm.c
gramm.c: gramm.y
	${YACC} -o $@ ${YFLAGS} gramm.y

lex.c: lex.l
	${LEX} ${LFLAGS} -o lex.c lex.l

clean:
	-rm ${PROG} *.o gramm.[hc] lex.c

.PHONY: all clean
