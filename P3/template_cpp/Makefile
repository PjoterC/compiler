all:
	bnfc -cpp CPP2.cf
	rm -f Test.C
	flex -Pcpp_ CPP2.l
	yacc -t -pcpp_ CPP2.y
	clang++-12 `llvm-config-12 --cxxflags --ldflags --system-libs --libs` -std=c++17 -g *.cpp *.C *.c -o compiler

clean:
	rm -f compiler

distclean: clean
	rm -f Absyn.* CPP2.l CPP2.y lex.cpp_.c lex.yy.c y.tab.c Parser.H Printer.* Skeleton.* Bison.* Buffer.* ParserError.*
