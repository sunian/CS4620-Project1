jcs5sb.out: jcs5sb.tlt scanner 
	./scanner < jcs5sb.tlt > jcs5sb.out

eins.out: eins.tlt scanner 
	./scanner < eins.tlt > eins.out

lex.yy.o:  lex.yy.c token.h symtab.h
	gcc -c lex.yy.c

lex.yy.c: turtle_jcs5sb.l
	flex turtle_jcs5sb.l
	
scanner:  lex.yy.o symtab.c
	gcc lex.yy.o symtab.c -lfl -o scanner 

clean:
	rm -f scanner lex.* 

