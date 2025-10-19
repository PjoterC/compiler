all:
	bnfc -cpp CPP.cf
	rm -f Skeleton.* Test.C
	flex -Pcpp_ CPP.l
	yacc -t -pcpp_ CPP.y
	clang++-12 -std=c++17 -g -static *.cpp *.C *.c -O3 -o compiler

clean:
	rm -f compiler

distclean: clean
	rm -f Absyn.* CPP.l CPP.y lex.yy.c y.tab.c Parser.H ParserError.H lex.cpp_.c Buffer.* Bison.H Printer.* Skeleton.*
