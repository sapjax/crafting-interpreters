#include "include/resolver.h"
#include <stdlib.h>
#include "include/hashtable.h"
#include "include/log.h"
#include "include/token.h"

Resolver* new_resolver() {
  Resolver* resolver = malloc(sizeof(*resolver));
  stack scopes = stack_create();
  resolver->scopes = scopes;
  resolver->cur_fn_type = F_NONE;
  return resolver;
};

void resolve(Resolver* resolver, Statement** stms) {
  resolve_statements(resolver, stms);
  stack_destroy(resolver->scopes);
};

void begin_scope(Resolver* resolver) {
  hash_table* scope = hash_table_create(8192, NULL);
  stack_push(resolver->scopes, scope);
};

void end_scope(Resolver* resolver) {
  hash_table* scope = stack_pop(resolver->scopes);
  hash_table_destroy(scope);
};

void resolve_block(Resolver* resolver, StatementBlock* stmt) {
  begin_scope(resolver);
  resolve_statements(resolver, stmt->stmts);
  end_scope(resolver);
};

void resolve_statements(Resolver* resolver, Statement** stmts) {
  for (int i = 0; stmts[i] != NULL; i++) {
    resolve_statement(resolver, stmts[i]);
  }
};

void resolve_statement(Resolver* resolver, Statement* stmt) {
  switch (stmt->type) {
    case STATEMENT_VAR:
      resolve_var_statement(resolver, stmt->u_stmt->var);
      break;
    case STATEMENT_FUNCTION:
      resolve_function_statement(resolver, stmt->u_stmt->function);
      break;
    case STATEMENT_CLASS:
      resolve_class_statement(resolver, stmt->u_stmt->class);
      break;
    case STATEMENT_BLOCK:
      resolve_block(resolver, stmt->u_stmt->block);
      break;
    case STATEMENT_EXPRESSION:
      resolve_expr(resolver, stmt->u_stmt->expr->expr);
      break;
    case STATEMENT_IF:
      resolve_expr(resolver, stmt->u_stmt->if_stmt->condition);
      resolve_statement(resolver, stmt->u_stmt->if_stmt->then_branch);
      if (stmt->u_stmt->if_stmt->else_branch != NULL) {
        resolve_statement(resolver, stmt->u_stmt->if_stmt->else_branch);
      }
      break;
    case STATEMENT_PRINT:
      resolve_expr(resolver, stmt->u_stmt->print->expr);
      break;
    case STATEMENT_RETURN:
      if (resolver->cur_fn_type == F_NONE) {
        log_error("Can't return from top-level code.");
        return;
      }
      if (stmt->u_stmt->return_stmt->value != NULL) {
        resolve_expr(resolver, stmt->u_stmt->return_stmt->value);
      }
      break;
    case STATEMENT_WHILE:
      resolve_expr(resolver, stmt->u_stmt->while_stmt->condition);
      resolve_statement(resolver, stmt->u_stmt->while_stmt->body);
      break;
    default:
      break;
  }
};

void resolve_var_statement(Resolver* resolver, StatementVar* stmt) {
  declare(resolver, stmt->name);
  if (stmt->initializer != NULL) {
    resolve_expr(resolver, stmt->initializer);
  }
  define(resolver, stmt->name);
};

void resolve_class_statement(Resolver* resolver, StatementClass* stmt) {
  ClassType enclosing_class_type = resolver->cur_class_type;
  resolver->cur_class_type = C_CLASS;
  declare(resolver, stmt->name);
  define(resolver, stmt->name);

  begin_scope(resolver);
  hash_table* scope = stack_top(resolver->scopes);
  hash_table_insert(scope, "this", (void*)1);

  for (int i = 0; stmt->methods[i] != NULL; i++) {
    resolve_function(resolver, stmt->methods[i]->u_stmt->function, F_METHOD);
  }

  end_scope(resolver);
  resolver->cur_class_type = enclosing_class_type;
};

void resolve_function_statement(Resolver* resolver, StatementFunction* stmt) {
  declare(resolver, stmt->name);
  define(resolver, stmt->name);
  resolve_function(resolver, stmt, F_FUNCTION);
};

void resolve_function(Resolver* resolver,
                      StatementFunction* stmt,
                      FunctionType type) {
  FunctionType enclosing_fn_type = resolver->cur_fn_type;
  resolver->cur_fn_type = type;

  begin_scope(resolver);
  int size = sizeof(stmt->params) / sizeof(stmt->params[0]);
  for (int i = 0; i < size - 1; i++) {
    Token* param = stmt->params[i];
    declare(resolver, param);
    define(resolver, param);
  }
  // don't use resolve_block here, cause we make params and body in the same
  // scope
  resolve_statements(resolver, stmt->body->u_stmt->block->stmts);
  end_scope(resolver);
  resolver->cur_fn_type = enclosing_fn_type;
};

void declare(Resolver* resolver, Token* name) {
  if (stack_is_empty(resolver->scopes))
    return;
  hash_table* scope = stack_top(resolver->scopes);

  if (hash_table_lookup(scope, name->lexeme) != NULL) {
    log_error("Variable %s already declared in this scope.", name->lexeme);
    return;
  }

  // use -1 to mark as declared but not initialized
  hash_table_insert(scope, name->lexeme, (void*)-1);
}

void define(Resolver* resolver, Token* name) {
  if (stack_is_empty(resolver->scopes))
    return;
  hash_table* scope = stack_top(resolver->scopes);
  // use 1 to mark as initialized
  hash_table_update(scope, name->lexeme, (void*)1);
}

void resolve_var_expr(Resolver* resolver, Expr* expr) {
  Token* name = expr->u_expr->variable->name;
  bool is_empty = stack_is_empty(resolver->scopes);
  if (!is_empty && hash_table_lookup(stack_top(resolver->scopes),
                                     name->lexeme) == (void*)-1) {
    log_error("Can't read local variable %s in its own initializer.",
              name->lexeme);
  }
  expr->u_expr->variable->depth = resolve_local(resolver, name);
}

// return depth for write to var or assign expr
int resolve_local(Resolver* resolver, Token* name) {
  for (int i = stack_size(resolver->scopes) - 1; i >= 0; i--) {
    hash_table* scope = stack_peek(resolver->scopes, i);
    if (hash_table_lookup(scope, name->lexeme) != NULL) {
      int depth = stack_size(resolver->scopes) - 1 - i;
      return depth;
    }
  }
  return -1;
};

void resolve_expr(Resolver* resolver, Expr* expr) {
  switch (expr->type) {
    case E_Assign: {
      resolve_expr(resolver, expr->u_expr->assign->value);
      int depth = resolve_local(resolver, expr->u_expr->assign->name);
      expr->u_expr->assign->depth = depth;
      break;
    }
    case E_Binary:
      resolve_expr(resolver, expr->u_expr->binary->left);
      resolve_expr(resolver, expr->u_expr->binary->right);
      break;
    case E_Get:
      resolve_expr(resolver, expr->u_expr->get->object);
      break;
    case E_Set:
      resolve_expr(resolver, expr->u_expr->set->value);
      resolve_expr(resolver, expr->u_expr->set->object);
      break;
    case E_This: {
      if (resolver->cur_class_type == C_NONE) {
        log_error("Can't use 'this' outside of a class.");
        return;
      }
      int depth = resolve_local(resolver, expr->u_expr->this->keyword);
      expr->u_expr->this->depth = depth;
      break;
    }
    case E_Call:
      resolve_expr(resolver, expr->u_expr->call->callee);
      Expr** args = expr->u_expr->call->arguments;
      int argc = sizeof(args) / sizeof(args[0]);
      for (int i = 0; i < argc - 1; i++) {
        resolve_expr(resolver, args[i]);
      }
      break;
    case E_Grouping:
      resolve_expr(resolver, expr->u_expr->grouping->expression);
      break;
    case E_Literal:
      break;
    case E_Logical:
      resolve_expr(resolver, expr->u_expr->logical->left);
      resolve_expr(resolver, expr->u_expr->logical->right);
      break;
    case E_Unary:
      resolve_expr(resolver, expr->u_expr->unary->right);
      break;
    case E_Variable:
      resolve_var_expr(resolver, expr);
      break;
  }
};