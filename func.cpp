/* Matt Van Veldhuizen
   3/21/15
   func.cpp
   CS 631
   Project
   C to MIPS assembly compiler
*/

#include "func.h"

string data = "\t.data\n";		//contains the data section of the MIPS assembly
string text = "\t.text\n\tjal main\n";  //contains the text section of the MIPS assembly
string filename = "c2mips.asm";		//the filename for the outout
static int elseCount = 0;		//defines the number of elses needed to correctly jump
static int loopCount = 0;		//defines the number of loops needed to correctly jump
static string scope = "";		//defines the scope for variables is a modifier to the name
bool isMain = false;			//defines if the current function is main or not
Register REGS;				//the available registers
static struct vector<struct symbol*> symtab; //the symbol table

//function that calls eval(...), treefree(...) and will write data/text to filename
void writeFile(struct ast *a) {
	eval(a);
	std::ofstream f;
	f.open(filename);
	f << data << "\n" << text << "\n";
	f.close();
	treefree(a);
}

//lookup/add new symbol to the symbol tree
struct symbol *lookup(char *sym) {
	string s(sym);
	//if symbol table is empty add the first symbol
	if(symtab.empty()) {
			struct symbol *sp = new symbol;
			sp->name = s;
			sp->val = 0;
			sp->func = NULL;
			sp->syms = NULL;
			symtab.push_back(sp);
			return sp;
	}
	//otherwise check each symbol for duplicates if no duplicates add to table
	else {
		for(unsigned int i = 0; i < symtab.size(); ++i) {
			if(symtab.at(i)->name == s)
				return symtab.at(i);

			else {
				struct symbol *sp = new symbol;
				sp->name = s;
				sp->val = 0;
				sp->func = NULL;
				sp->syms = NULL;
				symtab.push_back(sp);
				return sp;
			}
		}
	}

	yyerror("symbol table overflow\n");
	abort();
}

//set up new ast node
struct ast * new_ast(int nodetype, struct ast *l, struct ast *r) {
	struct ast *a = new ast;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = nodetype;
	a->l = l;
	a->r = r;
	return a;
}

//set up new incdec node
struct ast * new_incdec(int nodetype, struct ast *l) {
	struct incdec *a = new incdec;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = nodetype;
	a->v = l;
	return (struct ast *)a;
}

//set up new number node
struct ast * new_num(int i) {
	struct numval *a = new numval;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = 'N';
	a->i = i;
	return (struct ast *)a;
}

//set up new comparison node
struct ast * new_cmp(int cmptype, struct ast *l, struct ast *r) {
	struct ast *a = new ast;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = '0' + cmptype;
	a->l = l;
	a->r = r;
	return a;
}

//set up new function node
struct ast * new_func(struct symbol *s, struct symlist *params, struct ast *body) {
	struct fncall *a = new fncall;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = 'K';
	a->s = s;
	a->params = params;
	a->body = body;
	scope = s->name + "_";
	return (struct ast *)a;
}

//set up new function call node
struct ast * new_call(struct symbol *s, struct ast *params) {
	struct ufncall *a = new ufncall;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = 16;
	a->s = s;
	a->params = params;
	return (struct ast *)a;
}

//set up new variable reference node
struct ast * new_ref(struct symbol *s) {
	struct synref *a = new synref;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = 'T';
	a->s = s;
	return (struct ast *)a;
}

//set up new seen variable node
struct ast * see_ref(struct symbol *s) {
	struct synref *a = new synref;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = 'S';
	a->s = s;
	return (struct ast *)a;
}

//set up new variable assignment node
struct ast * new_asgn(struct symbol *s, struct ast *v) {
	struct synasgn *a = new synasgn;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = 'A';
	a->s = s;
	a->v = v;
	return (struct ast *)a;
}

//set up new variable equals node
struct ast * new_equal(struct symbol *s, struct ast *v) {
	struct synasgn *a = new synasgn;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = '=';
	a->s = s;
	a->v = v;
	return (struct ast *)a;
}

//set up new flow control node (if, if else, while)
struct ast * new_flow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el) {
	struct flow *a = new flow;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = nodetype;
	a->cond = cond;
	a->tl = tl;
	a->el = el;
	return (struct ast *)a;
}

//set up new flow control node (for)
struct ast * new_for(int nodetype, struct ast *var, struct ast *cond, struct ast *inc, struct ast *body) {
	struct forloop *a = new forloop;

	if(!a) {
		yyerror("out of space");
		exit(0);
	}
	a->nodetype = nodetype;
	a->cond = cond;
	a->var = var;
	a->inc = inc;
	a->body = body;
	return (struct ast *)a;
}

//set up new symbol list node
struct symlist * new_symlist(struct symbol *sym, struct symlist *next) {
	struct symlist *sl = new symlist;

	if(!sl) {
		yyerror("out of space");
		exit(0);
	}
	sl->sym = sym;
	sl->next = next;
	return sl;
}

//MIPS Assembly to store register value into variable name
void store(string name, string reg) {
	text.append("\tsw "+reg+", "+name+"\n");
}

//MIPS Assembly to create new variable with scope + variable name
string var(struct symbol *syn) {
	string name = scope + syn->name;
	data.append(name+":\t.word\n");
	return name;
}

//MIPS Assembly to create new function and set new scope
void calluser(struct fncall *f) {
	string name = f->s->name;	//function name
	text.append(name+":\n");

	struct symlist *sl;		//params list
	sl = f->params;
	//for the number of params take that register and store it into the new var
	for(; sl; sl = sl->next) {
		string reg = getReg('a');
		var(sl->sym);
		store(scope + sl->sym->name, reg);
	}
	//free the param registers
	remReg("$a0");
	remReg("$a1");
	remReg("$a2");
	remReg("$a3");
	//if we are in the main function
	if(name == "main")
		isMain = true;

	//call eval on the body of the function
	eval(f->body);
}

//MIPS Assembly to call a function with in the body of another function
string callfunc(struct ufncall *f) {
	//load params into correct registers
	string name = f->s->name;
	text.append("\tjal "+name+"\n");
	//save return values here
	return "";
}

//MIPS Assembly to load the number into registers
string load(string i) {
	string reg = getReg('t');
	text.append("\tli "+reg+", "+i+"\n");
	return reg;
}

//MIPS Assembly to load a variable into registers
string loadA(string a) {
	string reg = getReg('t');
	text.append("\tla "+reg+", "+a+"\n");
	string reg1 = getReg('t');
	text.append("\tlw "+reg1+", 0("+reg+")\n");
	remReg(reg);
	return reg1;
}

//MIPS Assembly to create new variable with a number
string asgn(struct symbol *syn, string i) {
	string name = scope + syn->name;
	data.append(name+":\t.word\t"+i+"\n");
	return name;
}

//MIPS Assembly to set a variable to an expression
string equal(struct symbol *syn, string v) {
	string name = scope + syn->name;

	try {
		//if a number load it into a register
		if(stoi(v)) {
			v = load(v);
			text.append("\tsw "+v+", "+name+"\n");
		}
	}
	catch (const std::invalid_argument &tmp) {
		//if not a register assume its a variable and load that into a register
		if(v[0] != '$') {
			string reg = getReg('t');
			text.append("\tla "+reg+", "+v+"\n");
			text.append("\tsw "+reg+", "+name+"\n");
			remReg(reg);
		}
		else
			text.append("\tsw "+v+", "+name+"\n");
	}
	return name;
}

//MIPS Assembly to shift a number by some amount
string shift(string op, string lhs, string rhs, bool eq) {
	string tmp = lhs;
	bool sw = false;
	//check to see if the left hand side of the operator is a number/register/variable
	try {
		//if a number load it into a register
		if(stoi(lhs)) {
			lhs = load(lhs);
		}
	}
	catch (const std::invalid_argument &tmp) {
		//if not a register assume its a variable and load that into a register
		if(lhs[0] != '$') {
			lhs = loadA(lhs);
			sw = true;
		}
	}
	string reg;
	//if its the >>= or <<= operator then the lhs is the final register
	if(eq)
		reg = lhs;
	else
		reg = getReg('t');

	//do the operation
	if(op == "<<") {
		text.append("\tsll "+reg+", "+lhs+", "+rhs+"\n");
	}
	else if(op == ">>") {
		text.append("\tsrl "+reg+", "+lhs+", "+rhs+"\n");
	}
	//if true store that value back into the variable
	if(sw) {
		store(tmp, reg);
	}
	remReg(lhs);
	return reg;
}

//if a number return its string version
string num(struct numval *a) {
	return to_string(a->i);
}

//MIPS Assembly to do the basic operations +,-,*,/
//and the bitwise operations ^,&,|,~
string op(int op, string lhs, string rhs, bool eq) {
	string tmpL = lhs;
	string tmpR = rhs;
	bool swR = false;
	bool swL = false;
	bool zero = false;
	//if lhs is zero for division check
	if(lhs == "0")
		zero = true;
	//check to see if the left hand side or right hand side of the operator is a number/register/variable
	try {
		//if rhs is a number then load it into a register
		if(stoi(rhs)) {
			rhs = load(rhs);
		}
	}
	catch (const std::invalid_argument &tmp) {
		//if rhs is a variable then load that variable into a register
		if(rhs[0] != '$') {
			rhs = loadA(rhs);
			swR = true;
		}
	}
	try {
		//if lhs is a number then load it into a register
		if(stoi(lhs)) {
			lhs = load(lhs);
		}
	}
	catch (const std::invalid_argument &tmp) {
		//if lhs is a variable then load that variable into a register
		if(lhs[0] != '$') {
			lhs = loadA(lhs);
			swL = true;
		}
	}

	string reg;
	//if the operation is of the form +=.-=.*=,/=.... then the lhs is the store register
	if(eq)
		reg = lhs;
	else
		reg = getReg('t');

	//do the operation
	switch (op) {
		case '+':
			text.append("\tadd "+reg+", "+lhs+", "+rhs+"\n");
			break;
		case '-':
			text.append("\tsub "+reg+", "+lhs+", "+rhs+"\n");
			break;
		case '*':
			text.append("\tmul "+reg+", "+lhs+", "+rhs+"\n");
			break;
		case '/':
			if(zero) {
				yyerror("divide by zero");
			}
			else {
				text.append("\tdiv "+reg+", "+lhs+", "+rhs+"\n");
			}
			break;
		case '%':
			text.append("\trem "+reg+", "+lhs+", "+rhs+"\n");
		case '|':
			text.append("\tor "+reg+", "+lhs+", "+rhs+"\n");
			break;
		case '&':
			text.append("\tand "+reg+", "+lhs+", "+rhs+"\n");;
			break;
		case '^':
			text.append("\txor "+reg+", "+lhs+", "+rhs+"\n");
			break;
		case '~':
			text.append("\tnot "+reg+", "+rhs+"\n");
			break;
	}
	//if lhs or rhs were variables store that number back into the variable
	if(swL) {
		store(tmpL, reg);
	}
	if(swR) {
		store(tmpR, reg);
	}
	//free up registers
	if(eq) {
		reg = lhs;
		remReg(rhs);
	}
	else {
		remReg(lhs);
		remReg(rhs);
	}

	return reg;
}

//MIPS Assembly to do the comparison operators
string cmp(int op, string lhs, string rhs) {
	string tmpL = lhs;
	string tmpR = rhs;
	bool swR = false;
	bool swL = false;
	//check to see if the left hand side or right hand side of the operator is a number/register/variable
	try {
		//if rhs is a number then load it into a register
		if(stoi(rhs)) {
			rhs = load(rhs);
		}
	}
	catch (const std::invalid_argument &tmp) {
		//if rhs is a variable then load that variable into a register
		if(rhs[0] != '$') {
			rhs = loadA(rhs);
			swR = true;
		}
	}
	try {
		//if lhs is a number then load it into a register
		if(stoi(lhs)) {
			lhs = load(lhs);
		}
	}
	catch (const std::invalid_argument &tmp) {
		//if lhs is a variable then load that variable into a register
		if(lhs[0] != '$') {
			lhs = loadA(lhs);
			swL = true;
		}
	}
	string reg = getReg('t');

	//do the operation
	switch(op) {
		//>
		case 1:
			text.append("\tsgt "+reg+", "+lhs+", "+rhs+"\n");
			break;
		//<
		case 2:
			text.append("\tslt "+reg+", "+lhs+", "+rhs+"\n");
			break;
		//!=
		case 3:
			text.append("\tsne "+reg+", "+lhs+", "+rhs+"\n");
			break;
		//==
		case 4:
			text.append("\tseq "+reg+", "+lhs+", "+rhs+"\n");
			break;
		//>=
		case 5:
			text.append("\tsge "+reg+", "+lhs+", "+rhs+"\n");
			break;
		//<=
		case 6:
			text.append("\tsle "+reg+", "+lhs+", "+rhs+"\n");
			break;
	}
	//if lhs, or rhs was a variable save the result back into the variable
	if(swL) {
		store(tmpL, reg);
	}
	if(swR) {
		store(tmpR, reg);
	}
	//free up registers
	remReg(lhs);
	remReg(rhs);

	return reg;
}

//MIPS Assembly to do the increment/decrement operators
string idop(int op, string v) {
	string reg = getReg('t');
	string tmp = v;
	bool sw = false;
	//check to see if v is a number/register/variable
	try {
		//if v is a number then load it into a register
		if(stoi(v)) {
			v = load(v);
		}
	}
	catch (const std::invalid_argument &tmp) {
		//if v is a variable then load that variable into a register
		if(v[0] != '$') {
			v = loadA(v);
			sw = true;
		}
	}
	//do the operation
	switch(op) {
		case '+':
			text.append("\taddi "+reg+", "+v+", 1\n");
			break;
		case '-':
			text.append("\tsubi "+reg+", "+v+", 1\n");
			break;
	}
	//if v was a variable then store that value back into the register
	if(sw) {
		store(tmp, reg);
	}
	//free register
	remReg(v);
	return reg;
}

//MIPS Assembly to do the if else flow control
void ifelse(struct flow *a) {
	//increment the elseCount and save it to tmp value
	//this is to correctly handle the jmp lables if multiple if else statements are found
	int elseTMP = ++elseCount;
	//if there is an else
	if(a->el) {
		text.append("\tbnez "+eval(a->cond)+", else"+to_string(elseTMP)+"\n");
		eval(a->tl);
		text.append("\tjal done"+to_string(elseTMP)+"\nelse"+to_string(elseTMP)+":");
		eval(a->el);
		text.append("done"+to_string(elseTMP)+":\n");
	}
	//if its just an if statement with no else
	else {
		text.append("\tbnez "+eval(a->cond)+", done"+to_string(elseTMP)+"\n");
		eval(a->tl);
		text.append("done"+to_string(elseTMP)+":\n");
	}
}

//MIPS Assembly to do the while loops flow control
void wLoops(struct flow *a) {
	//increment the loopCount and save it to tmp value
	//this is to correctly handle the jmp lables if multiple if else statements are found
	int loopTMP = ++loopCount;
	//check for a body (most likely there is but just to make sure)
	if(a->tl) {
		text.append("lpB"+to_string(loopTMP)+": ");
		eval(a->tl);
		text.append("\tbeqz "+eval(a->cond)+", lpE"+to_string(loopTMP)+"\n");
		text.append("\tj lpB"+to_string(loopTMP)+"\n");
		text.append("lpE"+to_string(loopTMP)+":\t");
	}
}

//MIPS Assembly to do the for loops flow control
void fLoops(struct forloop *a) {
	//increment the loopCount and save it to tmp value
	//this is to correctly handle the jmp lables if multiple if else statements are found
	int loopTMP = ++loopCount;
	//if any variables are define for( ***;... ; ...)
	eval(a->var);
	text.append("lpB"+to_string(loopTMP)+": ");
	//evaluate the body of the for loop
	eval(a->body);
	//check condtion for(... ; *** ; ...)
	text.append("\tbeqz "+eval(a->cond)+", lpE"+to_string(loopTMP)+"\n");
	//increment the variable for(... ; ... ; ***)
	eval(a->inc);
	text.append("\tj lpB"+to_string(loopTMP)+"\n");
	text.append("lpE"+to_string(loopTMP)+":\t");
}

//MIPS Assembly to handle the return statements from functions
void ret(string val) {
	//check to see if val is a number/register/variable
	try {
		//if val is a number then load it into the register
		if(stoi(val)) {
			val = load(val);
		}
	}
	catch (const std::invalid_argument &tmp) {
		//if val is a variable then load it into a registers
		if(val[0] != '$') {
			val = loadA(val);
		}
	}
	//if the program is in the main function call syscall with 10 to exit the program
	if(isMain) {
		text.append("\tli $v0, 10\n\tsyscall\n");
	}
	//otherwise move the value beging returned in the return register
	else {
		string reg = getReg('v');
		text.append("\tmove "+val+", "+reg+"\n");
		text.append("\tjr $ra\n");
	}
}

//MIPS Assembly to handle variable names with scope
string names(struct symbol *syn) {
	string name = scope + syn->name;
	return name;
}

//evaluate an AST
string eval(struct ast *a) {
	string v; //return value
	//check for bad nodetype
	if(!a) {
		yyerror("internal error, null eval");
	}
	//for each nodetype call the corresponding code gen function, save it to v and continue
	switch(a->nodetype) {
		case 'B': ret(eval(a->l)); break;							//return
		case 'N': v = num((struct numval *)a); break;						//number
		case 'S': v = names(((struct synasgn *)a)->s); break;					//variable/function name
		case 'T': v = var(((struct synasgn *)a)->s); break;					//new variable name
		case 'A': v = asgn(((struct synasgn *)a)->s, eval(((struct synasgn *)a)->v)); break;	//variable assignment
		case '=': v = equal(((struct synasgn *)a)->s, eval(((struct synasgn *)a)->v)); break;	//variable equals
		case '+': v = op('+', eval(a->l), eval(a->r), false); break;				//addition
		case '-': v = op('-', eval(a->l), eval(a->r), false); break;				//subtraction
		case '*': v = op('*', eval(a->l), eval(a->r), false); break;				//multiplication
		case '/': v = op('/', eval(a->l), eval(a->r), false); break;				//division
		case '%': v = op('%', eval(a->l), eval(a->r), false); break;				//modulus
		case '|': v = op('|', eval(a->l), eval(a->r), false); break;				//bitwise or
		case '&': v = op('&', eval(a->l), eval(a->r), false); break;				//bitwise and
		case '^': v = op('^', eval(a->l), eval(a->r), false); break;				//bitwise xor
		case '~': v = op('~', eval(a->l), eval(a->r), false); break;				//bitwise not
		case 'L': v = shift("<<", eval(a->l), eval(a->r), false); break;			//left shift
		case 'R': v = shift(">>", eval(a->l), eval(a->r), false); break;			//right shift
		case 'C': v = idop('+', eval(((struct incdec *)a)->v)); break;				//increment by 1 (++)
		case 'D': v = idop('-', eval(((struct incdec *)a)->v)); break;				//decrement by 1 (--)
		case '1': v = cmp(1, (eval(a->l)), eval(a->r)); break;					//grater than comparison
		case '2': v = cmp(2, (eval(a->l)), eval(a->r)); break;					//less than comparison
		case '3': v = cmp(3, (eval(a->l)), eval(a->r)); break;					//not equal to comparison
		case '4': v = cmp(4, (eval(a->l)), eval(a->r)); break;					//equal to comparison
		case '5': v = cmp(5, (eval(a->l)), eval(a->r)); break;					//greater than or equal to comparison
		case '6': v = cmp(6, (eval(a->l)), eval(a->r)); break;					//less than or equal to comparison
		case 7: v = op('+', eval(a->l), eval(a->r), true); break;				//+= operation
		case 8: v = op('-', eval(a->l), eval(a->r), true); break;				//-= operation
		case 9 : v = op('*', eval(a->l), eval(a->r), true); break;				//*= operation
		case 10: v = op('/', eval(a->l), eval(a->r), true); break;				// /= operation
		case 11: v = shift("<<", eval(a->l), eval(a->r), true); break;				//<<= operation
		case 12: v = shift(">>", eval(a->l), eval(a->r), true); break;				//>>= operation
		case 13: v = op('&', eval(a->l), eval(a->r), true); break;				//&= operation
		case 14: v = op('^', eval(a->l), eval(a->r), true); break;				//^= operation
		case 15: v = op('|', eval(a->l), eval(a->r), true); break;				//|= operation
		case 'I': ifelse((struct flow *)a); break;						//if else flow control
		case 'F': fLoops((struct forloop *)a); break;						//for loops flow control
		case 'W': wLoops((struct flow *)a); break;						//while loops flow control
		case 'E': eval(a->l); v = eval(a->r); break;						//multiple lists of statements
		case 'K': calluser((struct fncall *)a); break;						//new function
		case 16: v = callfunc((struct ufncall *)a); break;					//call function
		default: printf("internal error: bad node %c\n", a->nodetype);
	}
	return v;
}

//delete and free an AST
void treefree(struct ast *a) {
	switch(a->nodetype) {
		case '+':
		case '-':
		case '*':
		case '/':
		case '%':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case 'L':
		case 'R':
		case '|':
		case '&':
		case '^':
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 'E':
		case 'B':
			treefree(a->r);
		case 'C':
		case 'D':
		case '~':
		case 16:
			treefree(a->l);
		case 'N':
		case 'T':
		case 'S':
			break;
		case 'A':
		case '=':
			free( ((struct synasgn *)a)->v);
			break;
		case 'F':
			free( ((struct forloop *)a)->cond);
			free( ((struct forloop *)a)->var);
			if( ((struct forloop *)a)->inc) treefree( ((struct forloop*)a)->inc);
			if( ((struct forloop *)a)->body) treefree( ((struct forloop*)a)->body);
			break;
		case 'I':
		case 'W':
			free( ((struct flow *)a)->cond);
			if( ((struct flow *)a)->tl) treefree( ((struct flow*)a)->tl);
			if( ((struct flow *)a)->el) treefree( ((struct flow*)a)->el);
			break;
		case 'K':
			break;
		default: break; printf("internal error: free bad node %c\n", a->nodetype);
		}
	delete a;
}

//function that prints any errors if called
void yyerror(char const *s, ...) {
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "%d: error: ", yylineno);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
}

//function to set up the register allocation
void initRegs() {
	for(int i = 0; i < 2; i++) {
		REGS.v[i] = 0;
		REGS.a[i] = 0;
		REGS.k[i] = 0;
	}
	for(int i = 0; i < 10; i++)
		REGS.t[i] = 0;

	for(int i = 0; i < 8; i++)
		REGS.s[i] = 0;
}

//get register from avaliable lists
string getReg(char c) {
	string tmp;
	//based on which register type to get find one that is free, 0 = free, 1 = used
	switch (c) {
	case 'v':
		for(int i = 0; i < 2; i++) {
			if(!REGS.v[i]) {
				REGS.v[i] = 1;
				tmp = "$v"+to_string(i);
				break;
			}
		}
		break;
	case 'a':
		for(int i = 0; i < 2; i++) {
			if(!REGS.a[i]) {
				REGS.a[i] = 1;
				tmp = "$a"+to_string(i);
				break;
			}
		}
		break;
	case 't':
		for(int i = 0; i < 10; i++) {
			if(REGS.t[i] == 0) {
				REGS.t[i] = 1;
				tmp = "$t"+to_string(i);
				break;
			}
		}
		break;
	case 's':
		for(int i = 0; i < 8; i++) {
			if(!REGS.s[i]) {
				REGS.s[i] = 1;
				tmp = "$s"+to_string(i);
				break;
			}
		}
		break;
	case 'k':
		for(int i = 0; i < 2; i++) {
			if(!REGS.k[i]) {
				REGS.k[i] = 1;
				tmp = "$k"+to_string(i);
				break;
			}
		}
		break;
	}
	return tmp;
}

//remove registers from avaliable lists
void remReg(string reg) {
	char c = reg[1];
	switch (c) {
	case 'v':
		REGS.v[std::stoi(reg.substr(2))] = 0;
		break;
	case 'a':
		REGS.a[std::stoi(reg.substr(2))] = 0;
		break;
	case 't':
		REGS.t[std::stoi(reg.substr(2))] = 0;
		break;
	case 's':
		REGS.s[std::stoi(reg.substr(2))] = 0;
		break;
	case 'k':
		REGS.k[std::stoi(reg.substr(2))] = 0;;
		break;
	}
}