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

main.o:   error.h symbol.h code.h gramm.h
code.o:   error.h symbol.h code.h gramm.h
lex.o:    error.h symbol.h code.h gramm.h
gramm.o:  error.h symbol.h code.h
symbol.o: error.h symbol.h
error.o:  error.h

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
