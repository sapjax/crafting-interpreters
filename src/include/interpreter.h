#ifndef LOX_INTERPRETER_H
#define LOX_INTERPRETER_H
#include <ctype.h>
#include <stdbool.h>
#include "expression.h"
#include "hashtable.h"

typedef struct Env {
  char* name;
  struct Env* enclosing;
  hash_table* map;
} Env;

typedef struct Function {
  StatementFunction* declaration;
  Env* closure;
  bool is_initializer;
} Function;

typedef struct Class {
  char* name;
  hash_table* methods;
} Class;

typedef struct Instance {
  Class* class;
  hash_table* fields;
} Instance;

typedef union Value {
  char* string;
  double number;
  bool boolean;
  bool nil;
  Function* function;
  Class* class;
  Instance* instance;
} Value;

typedef enum ValueType {
  V_STRING,
  V_NUMBER,
  V_BOOL,
  V_NIL,
  V_FUNCTION,
  V_CLASS,
  V_INSTANCE
} ValueType;

typedef struct {
  ValueType type;
  Value* value;
} Object;

Env* new_env(Env* enclosing, char* name);

Object* env_define(Env* env, char* identifier, Object* value);
Object* env_update(Env* env, char* identifier, Object* value);
Object* env_lookup(Env* env, char* identifier);
void env_free(Env* env);
Env* find_declare_env(Env* env, int depth);

Value* new_value();

Object* new_object();
Object* new_function_obj(StatementFunction* declaration,
                         Env* closure,
                         bool is_initializer);

void interpret(Statement* statements[]);

void execute(Statement* statement, Env* env);

Object* evaluate(Expr* expr, Env* env);

Object* eval_variable(Expr* expr, Env* env);

Object* eval_literal(Expr* expr, Env* env);

Object* eval_unary(Expr* expr, Env* env);

Object* eval_grouping(Expr* expr, Env* env);

Object* eval_binary(Expr* expr, Env* env);

Object* eval_assign(Expr* expr, Env* env);

Object* eval_logical(Expr* expr, Env* env);

Object* eval_call(Expr* expr, Env* env);
Object* _eval_call_function(Object* callee, Expr* expr, Env* env);
Object* _eval_call_class(Object* callee, Expr* expr, Env* env);

Object* eval_get(Expr* expr, Env* env);

Object* eval_set(Expr* expr, Env* env);

Object* eval_this(Expr* expr, Env* env);

void eval_block(Statement* stmt, Env* env);

bool is_truthy(Object* obj);

bool is_logical_truthy(Object* obj);

void check_number_operand(Token* op, Object* left, Object* right);

char* stringify(Object* obj);

bool is_equal(Object* a, Object* b);

#endif