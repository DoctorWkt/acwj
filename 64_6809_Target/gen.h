/* gen.c */
int genlabel(void);
int genAST(struct ASTnode *n, int iflabel, int looptoplabel, int loopendlabel, int parentASTop);
void genpreamble();
void genpostamble();
void genfreeregs(int keepreg);
void genglobsym(struct symtable *node);
int genglobstr(char *strvalue);
