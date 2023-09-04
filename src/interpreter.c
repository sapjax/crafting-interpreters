#include "include/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/log.h"

Env* new_env(Env* enclosing) {
  Env* env = malloc(sizeof(*env));
  env->enclosing = enclosing;
  env->map = hash_table_create(8192, NULL);
  return env;
};

Object* env_define(Env* env, char* identifier, Object* obj) {
  hash_table_insert(env->map, identifier, obj);
  return obj;
};

Object* env_update(Env* env, char* identifier, Object* obj) {
  bool updated = hash_table_update(env->map, identifier, obj);
  if (!updated) {
    if (env->enclosing != NULL) {
      return env_update(env->enclosing, identifier, obj);
    } else {
      log_error("Undefined variable '%s'.", identifier);
    }
  }
  return obj;
};

Object* env_lookup(Env* env, char* identifier) {
  Object* obj = hash_table_lookup(env->map, identifier);
  if (obj == NULL) {
    if (env->enclosing != NULL) {
      return env_lookup(env->enclosing, identifier);
    } else {
      log_error("Undefined variable '%s'.", identifier);
    }
  }
  return obj;
};

void env_free(Env* env) {
  hash_table_destroy(env->map);
  free(env);
};

Value* new_value() {
  Value* v = malloc(sizeof(*v));
  v->number = 0;
  v->nil = true;
  return v;
};

Object* new_object() {
  Object* obj = malloc(sizeof(*obj));
  obj->type = V_NIL;
  obj->value = new_value();
  return obj;
};

void interpret(Statement** statements) {
  Env* env = new_env(NULL);
  for (int i = 0; statements[i] != NULL; i++) {
    Statement* stmt = statements[i];
    execute(stmt, env);
  }
  free(statements);
  env_free(env);
  // TODO: need to free Object* obj
};

void execute(Statement* statement, Env* env) {
  switch (statement->type) {
    case STATEMENT_EXPRESSION: {
      evaluate(statement->expr, env);
      break;
    }
    case STATEMENT_PRINT: {
      Object* obj = evaluate(statement->expr, env);
      char* str = stringify(obj);
      log_info("%s\n", str);
      // free(obj);
      break;
    }
    case STATEMENT_VAR: {
      if (statement->expr != NULL) {
        Object* obj = evaluate(statement->expr, env);
        env_define(env, statement->name->lexeme, obj);
      }
      break;
    }
    case STATEMENT_BLOCK: {
      Env* block_env = new_env(env);
      eval_block(statement, block_env);
      break;
    }
    default:
      break;
  }
};

void eval_block(Statement* stmt, Env* env) {
  for (int i = 0; stmt->block_stmts[i] != NULL; i++) {
    Statement* statement = stmt->block_stmts[i];
    execute(statement, env);
  }
  env_free(env);
};

Object* evaluate(Expr* expr, Env* env) {
  switch (expr->type) {
    case E_Literal:
      return eval_literal(expr, env);
    case E_Unary:
      return eval_unary(expr, env);
    case E_Grouping:
      return eval_grouping(expr, env);
    case E_Binary:
      return eval_binary(expr, env);
    case E_Variable:
      return eval_variable(expr, env);
    case E_Assign:
      return eval_assign(expr, env);
    default:
      return new_object();
  }
};

Object* eval_variable(Expr* expr, Env* env) {
  return env_lookup(env, expr->u_expr->variable->name->lexeme);
};

Object* eval_literal(Expr* expr, Env* env) {
  Token* token = expr->u_expr->literal->value;
  Literal* literal = token->literal;
  Object* obj = new_object();
  switch (token->type) {
    case STRING:
      obj->type = V_STRING;
      obj->value->string = (char*)malloc(strlen(literal->string) + 1);
      strcpy(obj->value->string, literal->string);
      return obj;
    case NUMBER:
      obj->type = V_NUMBER;
      obj->value->number = literal->number;
      return obj;
    case IDENTIFIER:
      free(obj);
      return env_lookup(env, literal->identifier);
    default:
      return obj;
  };
};

Object* eval_unary(Expr* expr, Env* env) {
  Object* right = evaluate(expr->u_expr->unary->right, env);
  Object* obj = new_object();

  switch (expr->u_expr->unary->op->type) {
    case MINUS:
      obj->type = V_NUMBER;
      obj->value->number = -right->value->number;
      break;
    case BANG:
      obj->type = V_BOOL;
      obj->value->boolean = !is_truthy(right);
      break;
    default:
      break;
  }
  free(right);
  return obj;
};

Object* eval_grouping(Expr* expr, Env* env) {
  return evaluate(expr->u_expr->grouping->expression, env);
};

Object* eval_binary(Expr* expr, Env* env) {
  Object* left = evaluate(expr->u_expr->binary->left, env);
  Object* right = evaluate(expr->u_expr->binary->right, env);
  Object* obj = new_object();
  switch (expr->u_expr->binary->op->type) {
    case GREATER:
      check_number_operand(expr->u_expr->binary->op, left, right);
      obj->type = V_BOOL;
      obj->value->boolean = left->value->number > right->value->number;
      break;
    case GREATER_EQUAL:
      check_number_operand(expr->u_expr->binary->op, left, right);
      obj->type = V_BOOL;
      obj->value->boolean = left->value->number >= right->value->number;
      break;
    case LESS:
      check_number_operand(expr->u_expr->binary->op, left, right);
      obj->type = V_BOOL;
      obj->value->boolean = left->value->number < right->value->number;
      break;
    case LESS_EQUAL:
      check_number_operand(expr->u_expr->binary->op, left, right);
      obj->type = V_BOOL;
      obj->value->boolean = left->value->number <= right->value->number;
      break;
    case BANG_EQUAL:
      obj->type = V_BOOL;
      obj->value->boolean = !is_equal(left, right);
      break;
    case EQUAL_EQUAL:
      obj->type = V_BOOL;
      obj->value->boolean = is_equal(left, right);
      break;
    case MINUS:
      check_number_operand(expr->u_expr->binary->op, left, right);
      obj->type = V_NUMBER;
      obj->value->number = left->value->number - right->value->number;
      break;
    case SLASH:
      check_number_operand(expr->u_expr->binary->op, left, right);
      obj->type = V_NUMBER;
      obj->value->number = left->value->number / right->value->number;
      break;
    case STAR:
      check_number_operand(expr->u_expr->binary->op, left, right);
      obj->type = V_NUMBER;
      obj->value->number = left->value->number * right->value->number;
      break;
    case PLUS:
      if (left->type == V_NUMBER && right->type == V_NUMBER) {
        obj->type = V_NUMBER;
        obj->value->number =
            (double)left->value->number + (double)right->value->number;
      } else if (left->type == V_STRING && right->type == V_STRING) {
        obj->type = V_STRING;
        size_t len =
            strlen(left->value->string) + strlen(right->value->string) + 1;
        obj->value->string = (char*)malloc(len);
        strcpy(obj->value->string, left->value->string);
        strcat(obj->value->string, right->value->string);
      } else {
        log_error("%s Operand must be all number or string. %d %d",
                  type_to_string(PLUS), left->type, right->type);
      }
      break;
    default:
      break;
  }
  free(left);
  free(right);
  return obj;
};

Object* eval_assign(Expr* expr, Env* env) {
  Object* obj = evaluate(expr->u_expr->assign->value, env);
  return env_update(env, expr->u_expr->assign->name->lexeme, obj);
};

void check_number_operand(Token* op, Object* left, Object* right) {
  if (left->type != V_NUMBER || right->type != V_NUMBER) {
    log_error("%s Operand must be a number. %d %d", type_to_string(op->type),
              left->type, right->type);
  }
};

char* stringify(Object* obj) {
  if (obj == NULL)
    return "nil";
  switch (obj->type) {
    case V_NIL:
      return "nil";
    case V_STRING: {
      char* str = (char*)malloc(strlen(obj->value->string) + 1);
      strcpy(str, obj->value->string);
      return str;
    }
    case V_BOOL:
      return obj->value->boolean == true ? "true" : "false";
    case V_NUMBER: {
      char buf[50];
      sprintf(buf, "%.g", obj->value->number);
      char* s = strdup(buf);
      return s;
    }
    default:
      return "nil";
  }
};

bool is_truthy(Object* obj) {
  if (obj == NULL)
    return false;
  if (obj->type == V_BOOL)
    return obj->value->boolean;
  return false;
};

bool is_equal(Object* a, Object* b) {
  if (a == NULL && b == NULL)
    return true;
  if (a == NULL || b == NULL)
    return false;

  if (a->type != b->type)
    return false;

  switch (a->type) {
    case V_BOOL:
      return a->value->boolean == b->value->boolean;
    case V_NIL:
      return a->value->nil == b->value->nil;
    case V_NUMBER:
      return a->value->number = b->value->number;
    case V_STRING:
      if (a->value->string == NULL && b->value->string == NULL)
        return true;
      else if ((a->value->string == NULL) || (b->value->string == NULL) ||
               (strcmp(a->value->string, b->value->string) != 0))
        return false;
    default:
      return false;
  }
};
