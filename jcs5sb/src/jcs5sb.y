
%{
#include <stdio.h>
#include "symtab.h"
%}

%union { int i; node *n; double d;}

%token GO TURN VAR JUMP
%token FOR STEP TO DO
%token COPEN CCLOSE
%token SIN COS SQRT
%token <d> FLOAT
%token <n> ID
%token <i> NUMBER
%token SEMICOLON PLUS MINUS TIMES DIV OPEN CLOSE ASSIGN
%token IF ELSE WHILE PROCEDURE
%token EQUALS GREATEROREQ LESSOREQ GREATERTHAN LESSTHAN
%token BOPEN
%token BCLOSE
%token THEN
%token COMMA
%token NOTEQUALS

%type <n> decl
%type <n> decllist

%%
program: head decllist stmtlist tail;

head: { printf("%%!PS Adobe\n\nnewpath\n0 0 moveto\n\n"
	       );
      };

tail: { printf("closepath\nstroke\n"); };

decllist: ;
decllist: decllist decl;

decl: VAR ID SEMICOLON { printf("/tlt%s 0 def\n",$2->symbol);} ;


stmtlist: ;
stmtlist: stmtlist stmt ;

stmt: IF OPEN condition CLOSE THEN 
	{printf("{\n");} 
	BOPEN stmtlist BCLOSE else;

else: { printf("} if\n");};
else: ELSE { printf("} {\n");} BOPEN stmtlist BCLOSE { printf("} ifelse\n");};

stmt: WHILE {printf("{ ");} OPEN condition CLOSE
	{printf("{} {exit} ifelse\n");}
	BOPEN stmtlist BCLOSE
	{ printf("} loop\n");};

stmt: ID ASSIGN expr SEMICOLON {printf("/tlt%s exch store\n",$1->symbol);} ;
stmt: GO expr SEMICOLON {printf("0 rlineto\n");};
stmt: JUMP expr SEMICOLON {printf("0 rmoveto\n");};
stmt: TURN expr SEMICOLON {printf("rotate\n");};

stmt: FOR ID ASSIGN expr 
          STEP expr
	  TO expr
	  DO {printf("{ /tlt%s exch store\n",$2->symbol);} 
	     stmt {printf("} for\n");};

stmt: COPEN stmtlist CCLOSE; 

condition: expr EQUALS expr {printf("eq\n");};
condition: expr NOTEQUALS expr {printf("ne\n");};
condition: expr GREATEROREQ expr {printf("ge\n");};
condition: expr LESSOREQ expr {printf("le\n");};
condition: expr GREATERTHAN expr {printf("gt\n");};
condition: expr LESSTHAN expr {printf("lt\n");};

expr: expr PLUS term { printf("add ");};
expr: expr MINUS term { printf("sub ");};
expr: term;

term: term TIMES factor { printf("mul ");};
term: term DIV factor { printf("div ");};
term: factor;

factor: MINUS atomic { printf("neg ");};
factor: PLUS atomic;
factor: SIN factor { printf("sin ");};
factor: COS factor { printf("cos ");};
factor: SQRT factor { printf("sqrt ");};
factor: atomic;



atomic: OPEN expr CLOSE;
atomic: NUMBER {printf("%d ",$1);};
atomic: FLOAT {printf("%f ",$1);};
atomic: ID {printf("tlt%s ", $1->symbol);};


%%
int yyerror(char *msg)
{
	extern int yylineno;	// defined and maintained in lex.c
	extern char *yytext;	// defined and maintained in lex.c
	fprintf(stderr,"bison error: %s at symbol \"%s\" on line %d\n",msg,yytext,yylineno);
	return 0;
}

int main(void)
{
	yyparse();
	return 0;
}

