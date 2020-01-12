// Function prototypes for all compiler files
// Copyright (c) 2019 Warren Toomey, GPL3

// scan.c
void reject_token(struct token *t);
int scan(struct token *t);

// tree.c
struct ASTnode *mkastnode(int op, int type,
			  struct ASTnode *left,
			  struct ASTnode *mid,
			  struct ASTnode *right, int intvalue);
struct ASTnode *mkastleaf(int op, int type, int intvalue);
struct ASTnode *mkastunary(int op, int type,
			    struct ASTnode *left, int intvalue);
void dumpAST(struct ASTnode *n, int label, int parentASTop);

// gen.c
int genlabel(void);
int genAST(struct ASTnode *n, int reg, int parentASTop);
void genpreamble();
void genpostamble();
void genfreeregs();
void genglobsym(int id);
int genglobstr(char *strvalue);
int genprimsize(int type);
void genreturn(int reg, int id);

// cg.c
void freeall_registers(void);
void cgpreamble();
void cgpostamble();
void cgfuncpreamble(int id);
void cgfuncpostamble(int id);
int cgloadint(int value, int type);
int cgloadglob(int id, int op);
int cgloadglobstr(int id);
int cgadd(int r1, int r2);
int cgsub(int r1, int r2);
int cgmul(int r1, int r2);
int cgdiv(int r1, int r2);
int cgshlconst(int r, int val);
int cgcall(int r, int id);
int cgstorglob(int r, int id);
void cgglobsym(int id);
void cgglobstr(int l, char *strvalue);
int cgcompare_and_set(int ASTop, int r1, int r2);
int cgcompare_and_jump(int ASTop, int r1, int r2, int label);
void cglabel(int l);
void cgjump(int l);
int cgwiden(int r, int oldtype, int newtype);
int cgprimsize(int type);
void cgreturn(int reg, int id);
int cgaddress(int id);
int cgderef(int r, int type);
int cgstorderef(int r1, int r2, int type);
int cgnegate(int r);
int cginvert(int r);
int cglognot(int r);
int cgboolean(int r, int op, int label);
int cgand(int r1, int r2);
int cgor(int r1, int r2);
int cgxor(int r1, int r2);
int cgshl(int r1, int r2);
int cgshr(int r1, int r2);

// expr.c
struct ASTnode *binexpr(int ptp);

// stmt.c
struct ASTnode *compound_statement(void);

// misc.c
void match(int t, char *what);
void semi(void);
void lbrace(void);
void rbrace(void);
void lparen(void);
void rparen(void);
void ident(void);
void fatal(char *s);
void fatals(char *s1, char *s2);
void fatald(char *s, int d);
void fatalc(char *s, int c);

// sym.c
int findglob(char *s);
int addglob(char *name, int type, int stype, int endlabel, int size);

// decl.c
void var_declaration(int type);
struct ASTnode *function_declaration(int type);
void global_declarations(void);

// types.c
int inttype(int type);
int parse_type(void);
int pointer_to(int type);
int value_at(int type);
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op);
