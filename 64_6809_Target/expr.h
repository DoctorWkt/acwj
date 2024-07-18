/* expr.c */
struct ASTnode *expression_list(int endtoken);
struct symtable *check_arg_vs_param(struct ASTnode *tree, struct symtable *param, struct symtable *funcptr);
struct ASTnode *binexpr(int ptp);
