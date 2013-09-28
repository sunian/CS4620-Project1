
all: turtle eins.ps

eins.ps: eins.tlt turtle
	./turtle < test1 > eins.ps

lex.yy.o:  lex.yy.c jcs5sb.tab.h symtab.h
	gcc -c lex.yy.c

lex.yy.c: turtle_jcs5sb.l
	flex turtle_jcs5sb.l
	
jcs5sb.tab.h: jcs5sb.y
	bison -d jcs5sb.y
	
jcs5sb.tab.c: jcs5sb.y
	bison -d jcs5sb.y
	
jcs5sb.tab.o: jcs5sb.tab.c symtab.h
	gcc -c jcs5sb.tab.c

turtle: jcs5sb.tab.o lex.yy.o symtab.c
	gcc lex.yy.o jcs5sb.tab.o symtab.c -lfl -o turtle

clean:
	rm -f turtle lex.* jcs5sb.tab.* *.o

