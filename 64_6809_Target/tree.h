/* tree.c */
struct ASTnode *mkastnode(int op, int type, struct symtable *ctype, struct ASTnode *left, struct ASTnode *mid, struct ASTnode *right, struct symtable *sym, int intvalue);
struct ASTnode *mkastleaf(int op, int type, struct symtable *ctype, struct symtable *sym, int intvalue);
struct ASTnode *mkastunary(int op, int type, struct symtable *ctype, struct ASTnode *left, struct symtable *sym, int intvalue);
void freeASTnode(struct ASTnode *tree);
void freetree(struct ASTnode *tree, int freenames);
struct ASTnode *loadASTnode(int id, int nextfunc);
void mkASTidxfile(void);
