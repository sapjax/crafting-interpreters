#include "include/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/log.h"

static Env* global_env = NULL;
void* latest_return_value = NULL;
bool function_returned = false;

void interpret(Statement** statements) {
  global_env = new_env(NULL, "global");
  for (int i = 0; statements[i] != NULL; i++) {
    Statement* stmt = statements[i];
    execute(stmt, global_env);
  }
  free(statements);
  env_free(global_env);
  free(latest_return_value);
};

Env* new_env(Env* enclosing, char* name) {
  Env* env = malloc(sizeof(*env));
  env->name = name;
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

Env* find_declare_env(Env* env, int depth) {
  if (depth == -1) {
    return global_env;
  }
  Env* tmp = env;
  for (int i = 0; i < depth; i++) {
    tmp = tmp->enclosing;
  }
  return tmp;
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

Object* new_function_obj(StatementFunction* declaration, Env* closure) {
  Object* obj = new_object();
  obj->type = V_FUNCTION;
  obj->value->function = malloc(sizeof(*obj->value->function));
  obj->value->function->declaration = declaration;
  obj->value->function->closure = closure;
  return obj;
};

// TODO: need to free Object* obj
void execute(Statement* statement, Env* env) {
  switch (statement->type) {
    case STATEMENT_EXPRESSION: {
      evaluate(statement->u_stmt->expr->expr, env);
      // NOTE: we can't free object here, cause in expression we may define a
      // variable in env,  if we free the object, the variable may be freed too
      // instead we free all objects in env_free.
      break;
    }
    case STATEMENT_PRINT: {
      Object* obj = evaluate(statement->u_stmt->print->expr, env);
      char* str = stringify(obj);
      log_info("%s\n", str);
      break;
    }
    case STATEMENT_VAR: {
      if (statement->u_stmt->var->initializer != NULL) {
        Object* obj = evaluate(statement->u_stmt->var->initializer, env);
        env_define(env, statement->u_stmt->var->name->lexeme, obj);
      }
      break;
    }
    case STATEMENT_BLOCK: {
      Env* block_env = new_env(env, "block");
      eval_block(statement, block_env);
      break;
    }
    case STATEMENT_IF: {
      Object* obj = evaluate(statement->u_stmt->if_stmt->condition, env);
      if (is_truthy(obj)) {
        execute(statement->u_stmt->if_stmt->then_branch, env);
      } else if (statement->u_stmt->if_stmt->else_branch != NULL) {
        execute(statement->u_stmt->if_stmt->else_branch, env);
      }
      free(obj);
      break;
    }
    case STATEMENT_WHILE: {
      while (
          is_truthy(evaluate(statement->u_stmt->while_stmt->condition, env))) {
        execute(statement->u_stmt->while_stmt->body, env);
      }
      break;
    }
    // function declare
    case STATEMENT_FUNCTION: {
      Object* obj = new_function_obj(statement->u_stmt->function, env);
      env_define(env, statement->u_stmt->function->name->lexeme, obj);
      break;
    }
    case STATEMENT_CLASS: {
      Object* obj = new_object();
      obj->type = V_CLASS;
      obj->value->class = (Class*)malloc(sizeof(Class));
      obj->value->class->name = statement->u_stmt->class->name->lexeme;
      obj->value->class->methods = hash_table_create(100, NULL);
      for (int i = 0; statement->u_stmt->class->methods[i] != NULL; i++) {
        Statement* method = statement->u_stmt->class->methods[i];
        Object* fnObj = new_function_obj(method->u_stmt->function, env);
        hash_table_insert(obj->value->class->methods,
                          method->u_stmt->function->name->lexeme, fnObj);
      }

      env_define(env, statement->u_stmt->class->name->lexeme, obj);
      break;
    }
    case STATEMENT_RETURN: {
      if (strcmp(env->name, "interpret") == 0) {
        log_error("Can't return from top-level code.");
        break;
      }
      Object* obj = evaluate(statement->u_stmt->return_stmt->value, env);
      latest_return_value = obj;
      break;
    }
    default:
      break;
  }
};

void eval_block(Statement* stmt, Env* env) {
  for (int i = 0; stmt->u_stmt->block->stmts[i] != NULL; i++) {
    if (function_returned)
      break;
    Statement* statement = stmt->u_stmt->block->stmts[i];
    execute(statement, env);
    // after return, we should break block to skip the rest statements
    if (statement->type == STATEMENT_RETURN) {
      function_returned = true;
      break;
    }
  }
  // NOTE: we can't free env here, cause in this block may define a function
  // this env will be used in function's closure
}

Object* evaluate(Expr* expr, Env* env) {
  if (expr == NULL)
    return new_object();
  switch (expr->type) {
    case E_Literal:
      return eval_literal(expr, env);
    case E_Unary:
      return eval_unary(expr, env);
    case E_Call:
      return eval_call(expr, env);
    case E_Get:
      return eval_get(expr, env);
    case E_Set:
      return eval_set(expr, env);
    case E_This:
      return eval_this(expr, env);
    case E_Grouping:
      return eval_grouping(expr, env);
    case E_Binary:
      return eval_binary(expr, env);
    case E_Variable:
      return eval_variable(expr, env);
    case E_Assign:
      return eval_assign(expr, env);
    case E_Logical:
      return eval_logical(expr, env);
    default:
      return new_object();
  }
};

Object* eval_variable(Expr* expr, Env* env) {
  Env* declare_env = find_declare_env(env, expr->u_expr->variable->depth);
  return env_lookup(declare_env, expr->u_expr->variable->name->lexeme);
};

Object* eval_this(Expr* expr, Env* env) {
  Env* declare_env = find_declare_env(env, expr->u_expr->variable->depth);
  return env_lookup(declare_env, "this");
};

Object* eval_get(Expr* expr, Env* env) {
  Object* obj = evaluate(expr->u_expr->get->object, env);
  if (obj->type == V_INSTANCE) {
    Object* value = hash_table_lookup(obj->value->instance->fields,
                                      expr->u_expr->get->name->lexeme);
    if (value != NULL) {
      return value;
    }
    // if not found in instance, try to find in class
    Class* class = obj->value->instance->class;
    if (class != NULL) {
      Object* method =
          hash_table_lookup(class->methods, expr->u_expr->get->name->lexeme);
      if (method != NULL) {
        Env* this_env = new_env(method->value->function->closure, "method");
        env_define(this_env, "this", obj);
        return new_function_obj(method->value->function->declaration, this_env);
      }
    }
    log_error("Undefined property '%s'.", expr->u_expr->get->name->lexeme);
  }
  log_error("Only instances have properties.");
  return NULL;
};

Object* eval_set(Expr* expr, Env* env) {
  Object* obj = evaluate(expr->u_expr->set->object, env);
  if (obj->type != V_INSTANCE) {
    log_error("Only instances have fields.");
  }
  Object* value = evaluate(expr->u_expr->set->value, env);
  hash_table_upsert(obj->value->instance->fields,
                    expr->u_expr->set->name->lexeme, value);
  return value;
};

Object* eval_literal(Expr* expr, Env* env) {
  Token* token = expr->u_expr->literal->value;
  Literal* literal = token->literal;
  Object* obj = new_object();
  switch (token->type) {
    case TRUE:
      obj->type = V_BOOL;
      obj->value->boolean = true;
      return obj;
    case FALSE:
      obj->type = V_BOOL;
      obj->value->boolean = false;
      return obj;
    case STRING:
      obj->type = V_STRING;
      obj->value->string = (char*)malloc(strlen(literal->string) + 1);
      strcpy(obj->value->string, literal->string);
      return obj;
    case NUMBER:
      obj->type = V_NUMBER;
      obj->value->number = literal->number;
      return obj;
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

Object* eval_call(Expr* expr, Env* env) {
  // callee is a function object, which return by env_lookup in eval_literal
  Object* callee = evaluate(expr->u_expr->call->callee, env);

  if (callee->type == V_CLASS) {
    Object* instance = new_object();
    instance->type = V_INSTANCE;
    instance->value->instance = malloc(sizeof(Instance));
    instance->value->instance->class = callee->value->class;
    instance->value->instance->fields = hash_table_create(100, NULL);
    return instance;
  }

  Object** arguments = malloc(sizeof(Object*) + sizeof(NULL));

  Env* closure = callee->value->function->closure;
  Env* fn_env = new_env(closure, "function");

  int i = 0;
  while (expr->u_expr->call->arguments[i] != NULL) {
    arguments = realloc(arguments, sizeof(Object*) * (i + 1) + sizeof(NULL));
    arguments[i] = evaluate(expr->u_expr->call->arguments[i], env);
    // set the function arguments to params
    env_define(fn_env, callee->value->function->declaration->params[i]->lexeme,
               arguments[i]);
    i++;
  }

  if (callee->type != V_FUNCTION && callee->type != V_CLASS) {
    log_error("Can only call functions and classes.");
  }

  // set a global variable for function return value
  latest_return_value = new_object();
  function_returned = false;
  eval_block(callee->value->function->declaration->body, fn_env);
  function_returned = false;
  free(arguments);
  free(fn_env);
  // check the return value
  return latest_return_value;
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
  // free(right);
  return obj;
};

Object* eval_assign(Expr* expr, Env* env) {
  Object* obj = evaluate(expr->u_expr->assign->value, env);
  Env* declare_env = find_declare_env(env, expr->u_expr->assign->depth);
  return env_update(declare_env, expr->u_expr->assign->name->lexeme, obj);
};

void check_number_operand(Token* op, Object* left, Object* right) {
  if (left->type != V_NUMBER || right->type != V_NUMBER) {
    log_error("%s Operand must be a number. %d %d", type_to_string(op->type),
              left->type, right->type);
  }
};

Object* eval_logical(Expr* expr, Env* env) {
  Object* left = evaluate(expr->u_expr->logical->left, env);
  if (expr->u_expr->logical->op->type == OR) {
    if (is_logical_truthy(left)) {
      return left;
    }
  } else {
    if (!is_logical_truthy(left)) {
      return left;
    }
  }
  free(left);
  return evaluate(expr->u_expr->logical->right, env);
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
      sprintf(buf, "%.1f", obj->value->number);
      char* s = strdup(buf);
      return s;
    }
    default:
      return "nil";
  }
};

// only true is true, else is false, used for if/while
bool is_truthy(Object* obj) {
  if (obj == NULL)
    return false;
  if (obj->type == V_BOOL)
    return obj->value->boolean;
  return false;
};

// only false and nil is logical false, used for or/and
bool is_logical_truthy(Object* obj) {
  if (obj == NULL) {
    return false;
  }
  if (obj->type == V_NIL) {
    return false;
  }
  if (obj->type == V_BOOL && obj->value->boolean == false) {
    return false;
  }
  return true;
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
