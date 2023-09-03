#ifndef LOX_INTERPRETER_H
#define LOX_INTERPRETER_H
#include <ctype.h>
#include <stdbool.h>
#include "expression.h"
#include "hashtable.h"

typedef union Value {
  char* string;
  double number;
  bool boolean;
  bool nil;
} Value;

typedef enum ValueType {
  V_STRING,
  V_NUMBER,
  V_BOOL,
  V_NIL,
} ValueType;

typedef struct {
  ValueType type;
  Value* value;
} Object;

typedef hash_table Env;

Value* new_value();

Object* new_object();

void interpret(Statement* statements[]);

void execute(Statement* statement, Env* env);

Object* evaluate(Expr* expr, Env* env);

Object* eval_literal(Expr* expr, Env* env);

Object* eval_unary(Expr* expr, Env* env);

Object* eval_grouping(Expr* expr, Env* env);

Object* eval_binary(Expr* expr, Env* env);

Object* eval_assign(Expr* expr, Env* env);

bool is_truthy(Object* obj);

void check_number_operand(Token* op, Object* left, Object* right);

char* stringify(Object* obj);

bool is_equal(Object* a, Object* b);

#endif