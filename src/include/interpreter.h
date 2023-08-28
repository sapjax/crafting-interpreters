#ifndef LOX_INTERPRETER_H
#define LOX_INTERPRETER_H
#include <ctype.h>
#include <stdbool.h>
#include "expression.h"

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

Value* new_value();

Object* new_object();

void interpret(Statement* statements[]);

void execute(Statement* statement);

Object* evaluate(Expr* expr);

Object* eval_literal(Expr* expr);

Object* eval_unary(Expr* expr);

Object* eval_grouping(Expr* expr);

Object* eval_binary(Expr* expr);

bool is_truthy(Object* obj);

void check_number_operand(Token* op, Object* left, Object* right);

char* stringify(Object* obj);

bool is_equal(Object* a, Object* b);

#endif