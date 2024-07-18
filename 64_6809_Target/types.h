/* types.c */
int inttype(int type);
int ptrtype(int type);
int pointer_to(int type);
int value_at(int type);
int typesize(int type, struct symtable *ctype);
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, struct symtable *rctype, int op);
