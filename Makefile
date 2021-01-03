PROG = hoc
OBJS = lex.o main.o bltin.o error.o symbol.o gramm.o args.o code.o

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

${OBJS}: hoc.h error.h
main.o: code.h symbol.h bltin.h gramm.h
gramm.o: code.h args.h symbol.h
code.o: code.h bltin.h args.h gramm.h
args.o: args.h
lex.o: symbol.h gramm.h
bltin.o: bltin.h
symbol.o:
error.o:
gramm.h: gramm.c

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -c $<

gramm.c: gramm.y
	${YACC} -o $@ ${YFLAGS} gramm.y

lex.c: lex.l
	${LEX} ${LFLAGS} -o lex.c lex.l

clean:
	-rm ${PROG} *.o gramm.[hc] lex.c

.PHONY: all clean
