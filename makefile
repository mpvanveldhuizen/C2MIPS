# Matt Van Veldhuizen
# 3/21/15
# makefile
# CS 631
# Project
# C to MIPS Assembly Compiler

all:	c2mips.lpp c2mips.ypp func.h
	bison -d c2mips.ypp
	flex c2mips.lpp
	g++ -std=c++11 -Wall func.cpp c2mips.tab.cpp lex.yy.c -o c2mips
	@echo "    Usage ./c2mips [input filename] -o [output filename]"
	@echo "      or  ./c2mips -o [output filename] [intput filename]"
	@echo "      or  ./c2mips -o [output filename]"
	@echo "      or  ./c2mips [input filename]"
	@echo "      or  ./c2mips"
	@echo
	@echo "      Use Mars4_5.jar to emulate the MIPS Assembly"
	@echo

clean:
	rm -rf c2mips c2mips.tab.cpp c2mips.tab.hpp lex.yy.c