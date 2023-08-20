#include "include/parser.h"
#include <stdio.h>
#include <stdlib.h>

Expr* expression(Parser* parser);

Expr* equality(Parser* parser);

Expr* comparison(Parser* parser);

Expr* term(Parser* parser);

Expr* factor(Parser* parser);

Expr* unary(Parser* parser);

Expr* primary(Parser* parser);

static Token* advance(Parser* parser);

static Token* peek(Parser* parser);

static Token* previous(Parser* parser);

static Token* consume(Parser* parser, TokenType type, char* message);

static void error(Token* token, char* message);

static void synchronize(Parser* parser);

static bool match(Parser* parser, TokenType* types, int size);

static bool check(Parser* parser, TokenType type);

static bool is_at_end(Parser* parser);

Parser* new_parser(Token** tokens, int num_tokens) {
  Parser* parser = (Parser*)malloc(sizeof(Token*) * num_tokens + sizeof(int));
  parser->tokens = tokens;
  parser->current = 0;
  return parser;
};

Expr* new_expr(UnTaggedExpr* u_expr, ExprType type) {
  Expr* expr = (Expr*)malloc(sizeof(Expr));
  expr->u_expr = u_expr;
  expr->type = type;
  return expr;
};

Expr* new_binary(Expr* left, Token* op, Expr* right) {
  ExprBinary* binary = malloc(sizeof(ExprBinary));
  binary->left = left;
  binary->op = op;
  binary->right = right;
  UnTaggedExpr* u_expr = (UnTaggedExpr*)malloc(sizeof(UnTaggedExpr));
  u_expr->binary = binary;
  return new_expr(u_expr, E_Binary);
};

Expr* new_unary(Token* op, Expr* right) {
  ExprUnary* unary = malloc(sizeof(ExprUnary));
  unary->op = op;
  unary->right = right;
  UnTaggedExpr* u_expr = (UnTaggedExpr*)malloc(sizeof(UnTaggedExpr));
  u_expr->unary = unary;
  return new_expr(u_expr, E_Unary);
};

Expr* new_literal(Token* value) {
  ExprLiteral* literal = malloc(sizeof(ExprLiteral));
  literal->value = value;
  UnTaggedExpr* u_expr = (UnTaggedExpr*)malloc(sizeof(UnTaggedExpr));
  u_expr->literal = literal;
  return new_expr(u_expr, E_Literal);
};

Expr* new_grouping(Expr* expression) {
  ExprGrouping* grouping = malloc(sizeof(ExprGrouping));
  grouping->expression = expression;
  UnTaggedExpr* u_expr = (UnTaggedExpr*)malloc(sizeof(UnTaggedExpr));
  u_expr->grouping = grouping;
  return new_expr(u_expr, E_Grouping);
};

Expr* parse(Parser* parser) {
  return expression(parser);
};

Expr* expression(Parser* parser) {
  return equality(parser);
};

Expr* equality(Parser* parser) {
  Expr* expr = comparison(parser);
  TokenType types[] = {BANG_EQUAL, EQUAL_EQUAL};
  while (match(parser, types, 2)) {
    Token* op = previous(parser);
    Expr* right = comparison(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* comparison(Parser* parser) {
  Expr* expr = term(parser);
  TokenType types[] = {GREATER, GREATER_EQUAL, LESS, LESS_EQUAL};
  while (match(parser, types, 4)) {
    Token* op = previous((parser));
    Expr* right = term(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* term(Parser* parser) {
  Expr* expr = factor(parser);
  TokenType types[] = {MINUS, PLUS};
  while (match(parser, types, 2)) {
    Token* op = previous((parser));
    Expr* right = factor(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* factor(Parser* parser) {
  Expr* expr = unary(parser);
  TokenType types[] = {SLASH, STAR};
  while (match(parser, types, 2)) {
    Token* op = previous((parser));
    Expr* right = unary(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* unary(Parser* parser) {
  TokenType types[] = {BANG, MINUS};
  if (match(parser, types, 2)) {
    Token* op = previous(parser);
    Expr* right = unary(parser);
    return new_unary(op, right);
  }
  return primary(parser);
};

Expr* primary(Parser* parser) {
  TokenType types_literal[] = {FALSE, TRUE, NIL, STRING, NUMBER};
  if (match(parser, types_literal, 5)) {
    return new_literal(previous(parser));
  }
  // matched group
  TokenType types_group_start[] = {LEFT_PAREN};
  if (match(parser, types_group_start, 1)) {
    Expr* expr = expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after expression.");
    return new_grouping(expr);
  }

  error(peek(parser), "Expect expression");
  return NULL;
}

bool is_at_end(Parser* parser) {
  return peek(parser)->type == E_O_F;
};

Token* peek(Parser* parser) {
  return parser->tokens[parser->current];
};

Token* previous(Parser* parser) {
  return parser->tokens[parser->current - 1];
};

Token* advance(Parser* parser) {
  if (!is_at_end(parser)) {
    parser->current++;
  }
  return previous(parser);
};

bool check(Parser* parser, TokenType type) {
  if (is_at_end(parser)) {
    return false;
  }
  return peek(parser)->type == type;
};

void error(Token* token, char* message) {
  if (token->type == E_O_F) {
    fprintf(stderr, "Parser Error: %s at end.\n", message);
  } else {
    fprintf(stderr, "Parser Error: %s of token: %s at %d.\n", message,
            token->lexeme, token->line);
  }
};

Token* consume(Parser* parser, TokenType type, char* message) {
  if (check(parser, type))
    return advance(parser);
  error(peek(parser), message);
  return NULL;
};

void synchronize(Parser* parser) {
  advance(parser);
  while (!is_at_end(parser)) {
    if (previous(parser)->type == SEMICOLON)
      return;

    switch (peek(parser)->type) {
      case CLASS:
      case FUN:
      case VAR:
      case FOR:
      case IF:
      case WHILE:
      case PRINT:
      case RETURN:
        return;
      default:
        advance(parser);
    }
  }
};

bool match(Parser* parser, TokenType* types, int size) {
  if (is_at_end((parser))) {
    return false;
  }
  for (int i = 0; i < size; i++) {
    if (check(parser, types[i])) {
      advance(parser);
      return true;
    }
  }
  return false;
};

// AST Printer
void print_expr(Expr* expr);
void print_binary(Expr* expr);
void print_unary(Expr* expr);
void print_grouping(Expr* expr);
void print_literal(Expr* expr);
void print_parenthesize(char* name, Expr** exprs, int size);

void print_ast(Expr* expr) {
  print_expr(expr);
}

void print_expr(Expr* expr) {
  if (expr == NULL) {
    return;
  }
  switch (expr->type) {
    case E_Binary:
      return print_binary(expr);
    case E_Unary:
      return print_unary(expr);
    case E_Grouping:
      return print_grouping(expr);
    case E_Literal:
      return print_literal(expr);
    default:
      return;
  }
}

void print_binary(Expr* expr) {
  Expr* exprs[] = {expr->u_expr->binary->left, expr->u_expr->binary->right};
  print_parenthesize(type_to_string(expr->u_expr->binary->op->type), exprs, 2);
}

void print_unary(Expr* expr) {
  Expr* exprs[] = {expr->u_expr->unary->right};
  print_parenthesize(type_to_string(expr->u_expr->unary->op->type), exprs, 1);
}

void print_grouping(Expr* expr) {
  Expr* exprs[] = {expr->u_expr->grouping->expression};
  print_parenthesize("group", exprs, 1);
}

void print_literal(Expr* expr) {
  Token* token = expr->u_expr->literal->value;
  switch (token->type) {
    case STRING:
      printf("%s", token->lexeme);
      break;
    case NUMBER:
      printf("%.1f", token->literal->number);
      break;
    default:
      printf("%s", type_to_string(token->type));
      return;
  }
}

void print_parenthesize(char* name, Expr** exprs, int size) {
  printf("(%s", name);
  for (int i = 0; i < size; i++) {
    printf(" ");
    print_expr(exprs[i]);
  }
  printf(")");
};