
#ifndef LOX_RESOLVER_H
#define LOX_RESOLVER_H
#include "expression.h"
#include "stack.h"

typedef enum _FunctionType { F_NONE, F_FUNCTION, F_METHOD } FunctionType;

typedef struct Resolver {
  stack scopes;
  FunctionType cur_fn_type;
} Resolver;

Resolver* new_resolver();
void resolve(Resolver* resolver, Statement** stms);
static void resolve_block(Resolver* resolver, StatementBlock* stmt);
static void resolve_statements(Resolver* resolver, Statement** stmts);
static void resolve_statement(Resolver* resolver, Statement* stmt);
static void resolve_var_statement(Resolver* resolver, StatementVar* stmt);
static void resolve_class_statement(Resolver* resolver, StatementClass* stmt);
static void resolve_function_statement(Resolver* resolver,
                                       StatementFunction* stmt);
static void resolve_function(Resolver* resolver,
                             StatementFunction* stmt,
                             FunctionType type);
static void resolve_expr(Resolver* resolver, Expr* expr);
static int resolve_local(Resolver* resolver, Token* name);
static void begin_scope(Resolver* resolver);
static void end_scope(Resolver* resolver);
static void declare(Resolver* resolver, Token* name);
static void define(Resolver* resolver, Token* name);

#endif