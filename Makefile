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
main.o: code.h symbol.h bltin.h y.tab.h
gramm.o: code.h args.h symbol.h
code.o: code.h bltin.h args.h
args.o: args.h
lex.o: symbol.h y.tab.h
bltin.o: bltin.h
symbol.o:
error.o:

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -c $<

gramm.c y.tab.h: gramm.y
	${YACC} ${YFLAGS} gramm.y
	mv y.tab.c gramm.c

lex.c: lex.l
	${LEX} ${LFLAGS} -o lex.c lex.l

clean:
	-rm ${PROG} y.* *.o gramm.c lex.c

.PHONY: all clean
