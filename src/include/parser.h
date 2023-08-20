#ifndef LOX_PARSER_H
#define LOX_PARSER_H
#include <stdbool.h>
#include "token.h"

typedef struct Parser {
  Token** tokens;
  int current;
} Parser;

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

typedef enum ExprType { E_Binary, E_Unary, E_Grouping, E_Literal } ExprType;

typedef union UnTaggedExpr {
  struct ExprBinary* binary;
  struct ExprUnary* unary;
  struct ExprLiteral* literal;
  struct ExprGrouping* grouping;
} UnTaggedExpr;

typedef struct Expr {
  ExprType type;
  union UnTaggedExpr* u_expr;
} Expr;

typedef struct ExprUnary {
  Token* op;
  struct Expr* right;
} ExprUnary;

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

Expr* new_expr(UnTaggedExpr* u_expr, ExprType type);
Expr* new_binary(Expr* left, Token* op, Expr* right);
Expr* new_unary(Token* op, Expr* right);
Expr* new_literal(Token* value);
Expr* new_grouping(Expr* expression);

/** rules of parser
 *
        expression     → equality ;
        equality       → comparison ( ( "!=" | "==" ) comparison )* ;
        comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
        term           → factor ( ( "-" | "+" ) factor )* ;
        factor         → unary ( ( "/" | "*" ) unary )* ;
        unary          → ( "!" | "-" ) unary
                                | primary ;
        primary        → NUMBER | STRING | "true" | "false" | "nil"
                                | "(" expression ")" ;
*/

Parser* new_parser(Token** tokens, int num_tokens);

Expr* parse(Parser* parser);

void print_ast(Expr* expr);

#endif