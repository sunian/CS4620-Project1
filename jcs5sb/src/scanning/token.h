
/* Tokens.  */
#define GO 258
#define TURN 259
#define VAR 260
#define JUMP 261
#define FOR 262
#define STEP 263
#define TO 264
#define DO 265
#define COPEN 266
#define CCLOSE 267
#define SIN 268
#define COS 269
#define SQRT 270
#define FLOAT 271
#define ID 272
#define NUMBER 273
#define SEMICOLON 274
#define PLUS 275
#define MINUS 276
#define TIMES 277
#define DIV 278
#define OPEN 279
#define CLOSE 280
#define ASSIGN 281
#define IF 282
#define ELSE 283
#define WHILE 284
#define PROCEDURE 285
#define EQUALS 286
#define GREATEROREQ 287
#define LESSOREQ 288
#define GREATERTHAN 289
#define LESSTHAN 290
#define BOPEN 291
#define BCLOSE 292
#define IFTHEN 293
#define COMMA 294
#define NOTEQUALS 295



typedef union YYSTYPE
{ int i; node *n; double d;}
        YYSTYPE;
YYSTYPE yylval;

