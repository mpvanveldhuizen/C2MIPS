# CtoMIPS
### Introduction
For the project I implemented a C to MIPS Assembly compiler. The following sections describe the parser, lexer, and the associated AST functions that were written. At this point only integer numbers and operations were implemented due to the complexity of getting floating point numbers and operations included. Therefor the basic operations (+,-,*,/, etc), have been written with control flow (if, else, while, for). And lastly functions have been implemented but to a limited sense. They cannot pass any arguments between functions, however returning from functions is possible.

### Lexer
The following lists what are the only operators begin matched, the full lexer can be found in the c2mips.lpp file included with this document:

| Symbol | Description |
| --- | --- |
| `+` | plus for addition |
| `-` | minus for subtraction |
| `*` | star for multiplication |
| `/` | slash for division |
| `%` | percent for modulus |
| `|` | vertical line for bitwise or |
| `&` | ampersand for bitwise and |
| `^` | carrot for bitwise XOR |
| `~` | tilde for bitwise not |
| `=` | equal for assignment |
| `,` | comma for lists | 
| `(` | left parenthesis for grouping |
| `)` | right parenthesis for grouping |
| `;` | semi colon for statement ending |
| `{` | left bracket for statement grouping |
| `}` | right bracket for statement grouping |
| `<<` | double left carrot for shift left |
| `>>` | double right carrot for shift right |
| `>` | right carrot for grater than comparison |
| `<` | left carrot for less than comparison |
| `!=` | exclamation equal for not equal comparison |
| `==` | double equal for equal to comparison |
| `>=` | right carrot equal for grater than or equal to comparison |
| `<=` | left carrot equal for less than or equal to comparison |
| `+=` | plus equal for increment by amount |
| `-=` | minus equal for decrement by amount |
| `*=` | star equal for multiply by amount |
| `/=` | slash equal for divide by amount |
| `<<=` | double left carrot for shift by amount |
| `>>=` | double right carrot for shift by amount |
| `&=` | ampersand equal for bitwise and by amount |
| `|=` | vertical line equal for bitwise or by amount |
| `^=` | carrot equal for bitwise XOR by amount |

The following list is the keywords being matched:

| Keyword | Description |
| --- | --- |
| `if` | for if control flow statements |
| `else` | for else control flow statements |
| `while` | for while loop statements |
| `for` | for for loop statements |
| `return` | for returning from functions |
| `int` | declaring integer variables |

The following list is are the regular expressions that are being looked for:

| Regular Expression | Description |
| --- | --- |
| `[a-zA-Z][a-zA-Z0-9]*` | variable/function names |
| `[0-9]+` | integers |
| `([0-9]*”."[0-9]+|[0-9]+”.”){EXP}? | “.”?[0-9]+{EXP}?` `EXP = ([Ee][-+]?[0-9]+)` | floating point numbers`*`  |
| `[ \t\n]` | ignoring white space |
| `“/* */”` | ignoring comments |
| `.` | anything else |

`*` however they are not supported in the grammar at this time

### Parser
The following is grammar that was used in order to generate the MIPS assembly, the full parser can be found in the c2mips.ypp file included with this document:

`clist -> e | clist func | clist error`
  - `clist` is the start of the grammar it can go to nothing, multiple func or an error state

`func -> NINT NAME ‘(‘ symlist ’)’ ‘{‘ stmtlist ‘}’`
  - `func` defines the start of a function definition, which has arguments `symlist` and a body `stmtlist`

`stmtlist -> stmt stmtlist | stmt`
  - `stmtlist` defines multiple statements or a single statement `stmt`

```
stmt -> IF ‘(‘ cond ’)’ ‘{‘ stmt ’}’
      | IF ‘(‘ cond ’)’ ‘{‘ stmt ’}’ ELSE ‘{‘ stmt ’}’
      | WHILE ‘(‘ cond ’)’ ‘{‘ stmt ’}’
      | FOR ‘(‘ assign_var ‘;’ cond ‘;’ exp ‘)’ ‘{‘ stmt ’}’
      | RETURN explist
      | explist
```
  - `stmt` defines the flow control `if`, `else`, `while`, `for`, which has a set of conditions `cond` and a body `stmt` and the return statement for the functions. In addition it will call `explist` or a list of expressions.

`cond -> exp CMP exp`
  - `cond` defines an expression that is compared to another expression, using one of the comparison operators
  
`explist -> exp ‘;’ explist | exp ‘;’`
  - `explist` defines multiple expressions ending in a semi-colon

```
exp -> exp SRT exp
     | exp SLT exp
     | exp '+' exp
     | exp '-' exp
     | exp '*' exp
     | exp '/' exp
     | exp '%' exp
     | exp '|' exp
     | exp '&' exp}
     | exp '^' exp
     | '~' exp
     | '(' exp ')'
     | INC exp
     | DEC exp
     | exp PEQ exp
     | exp MEQ exp
     | exp TEQ exp
     | exp DEQ exp
     | exp LSEQ exp
     | exp RSEQ exp
     | exp AEQ exp
     | exp XEQ exp
     | exp OEQ exp
     | NAME
     | NAME '(' varlist ')'
     | number
     | declare_var
     | assign_var
```
  - `exp` defines the set of operations that can be an expression `+`, `-`, `*`, `/`, etc. It also defines how functions and variables are called, as well as defining how variables are defined and assigned
  
`varlist -> exp ‘,’ varlist | exp | e`
    - `varlist` defines multiple variables that would be in a function call

`assign_var -> NAME ‘=’ exp`
  - `assign_var` defines how a variable can get assigned some expression

`declare_var -> NINT NAME | NINT NAME ‘=’ exp`
  - `declare_var` defines how a new variable is declared with or without an expression

`number -> INT`
  - `number` defines only integers at this moment

`symlist -> NINT NAME | symlist ‘,’ NINT NAME | e`
  - `symlist` defines a list of variable names to be used in function declarations

### AST Functions
The following list are the functions and structs used to generate and evaluate the AST, the full AST function and structure list can be found in the func.h and func.cpp files included with this document.

Because of how MIPS uses registers it is necessary to handle register allocation. MIPS only allows for the use of 32 temporary registers, and requires that the registers that are no longer needed be freed up, so that they can be used again. The `remReg(...)`, `getReg(...)` and `initReg(...)` function handle the initialization, removal and retrieval of registers. I would like to point out that this is not a perfect solution, and it can run out of registers.

Each of the following structures has an associated `new_*` function, where it creates a new node of that type.
```
struct ast {
  int nodetype;
  struct ast *l;
  struct ast *r;
}
```
This is the main struct type used in the ast tree, it has a nodetype which defines what operation is going to be applied to this node, and an ast l, r, which are used to track what is on the left of the node, and on the right of the node.
```
struct fncall {
  int nodetype;
  struct symlist *params;
  struct ast *body;
  struct symbol *s;
}
```
This struct defines any function that gets created, each struct has a nodetype, which acts exactly like the previous nodetypes. A symlist of parameters, these are the parameters that are getting passed into the function, the body of the function which is another ast node which defines anything else in thats in the body of the function. And a symbol s, or the name of the function.
```
struct flow {
  int nodetype;
  struct ast *cond;
  struct ast *tl;
  struct ast *el;
}
```
This struct defines the basic flow control elements, such as IF, IF ELSE, and WHILE statements. It contains a nodetype that defines which of the flow control statements is being called. ASTs for the condition (cond), the statement for after the if (tl) and else (el) if an else is needed.
```
struct forloop {
  int nodetype;
  struct ast *cond;
  struct ast *var;
  struct ast *inc;
  struct ast *body;
}
```
This struct defines the flow control for for loops. It has a nodetype, ast node for variable declaration, ast node for the condition, ast node for the increment, and an ast node for the body of the for loop.
```
struct numval {
  int nodetype;
  int i;
}
```
This struct defines a new number.
```
struct synref {
  int nodetype;
  struct symbol *s;
}
```
This struct defines a new symbol reference, namely when a variable is called.
```
struct synasgn {
  int nodetype;
  struct symbol;
  struct ast *v;
}
```
This struct defines a new variable assignment. It has a symbol and a ast v which goes to some expression.
```
struct incdec {
  int nodetype;
  struct ast *v;
}
```
This struct defines a new increment (++) or decrement (--) operations.
```
struct ufncall {
  int nodetype;
  struct ast *params;
  struct symbol *s;
}
```
This struct defines when a function gets called. It has an ast params and a symbol that is the functions name.

The last important AST function is the eval function. This function takes an ast node and depending on the nodetype, it will call the corresponding code gen function. For example, it the nodetype is `+` it will call the function `op(...)`, and will apply the correct MIPS Assembly instruction. And once the whole tree has been evaluated it will then delete all of the nodes in the tree using the treefree function.
