/* libmain - flex run-time support library "main" function */

/* $Header: /home/mike/src/cvs/netbsd/src/usr.bin/lex/Attic/libmain.c,v 1.1 1994/02/04 19:15:29 jtc Exp $ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];
	{
	return yylex();
	}
