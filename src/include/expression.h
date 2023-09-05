#ifndef LOX_EXPR_H
#define LOX_EXPR_H
#include <stdbool.h>
#include "token.h"

/** grammar of expressions
        expression     → literal
                                | unary
                                | binary
                                | grouping ;

        literal        → NUMBER | STRING | "true" | "false" | "nil" ;
        grouping       → "(" expression ")" ;
        unary          → ( "-" | "!" ) expression ;
        binary         → expression operator expression ;
        operator       → "==" | "!=" | "<" | "<=" | ">" | ">="
                                | "+"  | "-"  | "*" | "/" ;
*/

typedef enum ExprType {
  E_Binary,
  E_Unary,
  E_Grouping,
  E_Literal,
  E_Variable,
  E_Assign
} ExprType;

typedef union UnTaggedExpr {
  struct ExprBinary* binary;
  struct ExprUnary* unary;
  struct ExprLiteral* literal;
  struct ExprGrouping* grouping;
  struct ExprVariable* variable;
  struct ExprAssign* assign;
} UnTaggedExpr;

typedef struct Expr {
  ExprType type;
  union UnTaggedExpr* u_expr;
} Expr;

typedef struct ExprUnary {
  Token* op;
  struct Expr* right;
} ExprUnary;

typedef struct ExprVariable {
  Token* name;
} ExprVariable;

typedef struct ExprGrouping {
  struct Expr* expression;
} ExprGrouping;

typedef struct ExprLiteral {
  Token* value;
} ExprLiteral;

typedef struct ExprBinary {
  struct Expr* left;
  Token* op;
  struct Expr* right;
} ExprBinary;

typedef struct ExprAssign {
  Token* name;
  struct Expr* value;
} ExprAssign;

typedef enum StatementType {
  STATEMENT_EXPRESSION,
  STATEMENT_PRINT,
  STATEMENT_VAR,
  STATEMENT_BLOCK,
  STATEMENT_IF,
  STATEMENT_WHILE,
  STATEMENT_FUNCTION,
  STATEMENT_RETURN
} StatementType;

typedef struct StatementExpression {
  Expr* expr;
} StatementExpression;

typedef struct StatementPrint {
  Expr* expr;
} StatementPrint;

typedef struct StatementVar {
  Token* name;
  Expr* initializer;
} StatementVar;

typedef struct StatementBlock {
  struct Statement** stms;
} StatementBlock;

typedef struct StatementIf {
  Expr* condition;
  struct Statement* then_branch;
  struct Statement* else_branch;
} StatementIf;

typedef union UnTaggedStatement {
  struct StatementExpression* expr;
  struct StatementPrint* print;
  struct StatementVar* var;
  struct StatementBlock* block;
  struct StatementIf* if_stmt;
} UnTaggedStatement;

typedef struct Statement {
  StatementType type;
  union UnTaggedStatement* u_stmt;
} Statement;

#endif