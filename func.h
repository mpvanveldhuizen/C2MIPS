/* Matt Van Veldhuizen
   3/21/15
   func.h
   CS 631
   Project
   C to MIPS assembly compiler
*/

#ifndef FUNC_H
#define FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <string.h>
#include <fstream>
#include <stdexcept>
#include <vector>
using std::vector;
using std::string;
using std::to_string;

extern string data;	//contains the data section of the MIPS assembly
extern string text;	//contains the text section of the MIPS assembly
extern string filename; //the filename for the outout
extern int yylineno;	//the current line number the parser/lexer is on

//struct that holds the symbol of any given variable, function
struct symbol {
	string name;
	int val;
	struct ast *func;
	struct symlist *syms;
};

//function that is used when a function is defined
extern void calluser(struct fncall *);

//function that calls eval(...), treefree(...) and will write data/text to filename
void writeFile(struct ast *);

//function to set up the register allocation
void initRegs();

//function that prints any errors if called
void yyerror(char const *s, ...);

//set up new ast node
struct ast *new_ast(int nodetype, struct ast *l, struct ast *r);

//set up new incdec node
struct ast *new_incdec(int nodetype, struct ast *v);

//set up new comparison node
struct ast *new_cmp(int cmptype, struct ast *l, struct ast *r);

//set up new function node
struct ast *new_func(struct symbol *s, struct symlist *param, struct ast *body);

//set up new variable reference node
struct ast *new_ref(struct symbol *s);

//set up new seen variable node
struct ast *see_ref(struct symbol *s);

//set up new variable assignment node
struct ast *new_asgn(struct symbol *s, struct ast *v);

//set up new variable equals node
struct ast *new_equal(struct symbol *s, struct ast *v);

//set up new number node
struct ast *new_num(int d);

//set up new flow control node (if, if else, while)
struct ast *new_flow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el);

//set up new flow control node (for)
struct ast *new_for(int nodetype, struct ast *var, struct ast *cond, struct ast *inc, struct ast *body);

//set up new symbol list node
struct symlist *new_symlist(struct symbol *sym, struct symlist *next);

//set up new function call node
struct ast *new_call(struct symbol *s, struct ast *params);

//evaluate an AST
string eval(struct ast *);

//delete and free an AST
void treefree(struct ast *);

//remove registers from avaliable lists
void remReg(string reg);

//get register from avaliable lists
string getReg(char c);

//lookup/add new symbol to the symbol tree
struct symbol *lookup(char *sym);

//delete from the symbol tree
void symlistfree(struct symlist *sl);

//defines a symbol list tree
struct symlist {
	struct symbol *sym;
	struct symlist *next;
};

//defines the avaliable registers 0 = free to use 1 = taken go to the next one
struct Register {
	int v[2];
	int a[4];
	int t[10];
	int s[8];
	int k[2];
};
extern Register REGS;

//nodes in the abstract syntax tree
struct ast {
	int nodetype;
	struct ast *l;
	struct ast *r;
};

struct fncall {
	int nodetype;
	struct symlist *params;
	struct ast *body;
	struct symbol *s;
};

struct flow {
	int nodetype;
	struct ast *cond;
	struct ast *tl;
	struct ast *el;
};

struct forloop {
	int nodetype;
	struct ast *cond;
	struct ast *var;
	struct ast *inc;
	struct ast *body;
};

struct numval {
	int nodetype;
	int i;
};

struct synref {
	int nodetype;
	struct symbol *s;
};

struct synasgn {
	int nodetype;
	struct symbol *s;
	struct ast *v;
};

struct incdec {
	int nodetype;
	struct ast *v;
};

struct ufncall {
	int nodetype;
	struct ast *params;
	struct symbol *s;
};

#endif