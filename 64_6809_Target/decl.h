/* decl.c */
int parse_type(struct symtable **ctype, int *class);
int parse_stars(int type);
int parse_cast(struct symtable **ctype);
int parse_literal(int type);
int declaration_list(struct symtable **ctype, int class, int et1, int et2, struct ASTnode **gluetree);
void global_declarations(void);
