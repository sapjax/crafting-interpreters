#include "include/parser.h"
#include <stdio.h>
#include <stdlib.h>

Statement* statement(Parser* parser);
Statement* print_statement(Parser* parser);
Statement* expression_statement(Parser* parser);
Statement* new_statement(StatementType type, Expr* expr, Token* name);

Statement* declaration(Parser* parser);
Statement* var_declaration(Parser* parser);

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

static bool match(Parser* parser, TokenType* types);
static bool check(Parser* parser, TokenType type);
static bool is_at_end(Parser* parser);

Expr* new_expr(UnTaggedExpr* u_expr, ExprType type);
Expr* new_binary(Expr* left, Token* op, Expr* right);
Expr* new_unary(Token* op, Expr* right);
Expr* new_literal(Token* value);
Expr* new_grouping(Expr* expression);
Expr* new_variable(Token* value);

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

UnTaggedExpr* new_untagged_expr() {
  UnTaggedExpr* u_expr = (UnTaggedExpr*)malloc(sizeof(UnTaggedExpr));
  return u_expr;
}

Expr* new_binary(Expr* left, Token* op, Expr* right) {
  ExprBinary* binary = malloc(sizeof(ExprBinary));
  binary->left = left;
  binary->op = op;
  binary->right = right;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->binary = binary;
  return new_expr(u_expr, E_Binary);
};

Expr* new_unary(Token* op, Expr* right) {
  ExprUnary* unary = malloc(sizeof(ExprUnary));
  unary->op = op;
  unary->right = right;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->unary = unary;
  return new_expr(u_expr, E_Unary);
};

Expr* new_literal(Token* value) {
  ExprLiteral* literal = malloc(sizeof(ExprLiteral));
  literal->value = value;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->literal = literal;
  return new_expr(u_expr, E_Literal);
};

Expr* new_grouping(Expr* expression) {
  ExprGrouping* grouping = malloc(sizeof(ExprGrouping));
  grouping->expression = expression;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->grouping = grouping;
  return new_expr(u_expr, E_Grouping);
};

Expr* new_variable(Token* name) {
  ExprVariable* variable = malloc(sizeof(ExprVariable));
  variable->name = name;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->variable = variable;
  return new_expr(u_expr, E_Variable);
};

Statement** parse(Parser* parser) {
  Statement** stmts = malloc(sizeof(Statement) * 0 + sizeof(NULL));
  int i = 0;
  while (!is_at_end(parser)) {
    stmts = realloc(stmts, sizeof(Statement) * i + sizeof(NULL));
    stmts[i] = declaration(parser);
    i++;
  }
  // add NULL at end for loop stop flag.
  stmts[i] = NULL;
  return stmts;
};

Statement* declaration(Parser* parser) {
  if (match(parser, (TokenType[]){VAR, -1}))
    return var_declaration(parser);
  return statement(parser);
};

Statement* var_declaration(Parser* parser) {
  Token* name = consume(parser, IDENTIFIER, "Expect variable name.");
  Expr* initializer = NULL;
  if (match(parser, (TokenType[]){EQUAL, -1})) {
    initializer = expression(parser);
  }
  consume(parser, SEMICOLON, "Expect ';' after variable declaration.");
  return new_statement(STATEMENT_VAR, initializer, name);
};

Statement* new_statement(StatementType type, Expr* expr, Token* name) {
  Statement* stmt = malloc(sizeof(Statement));
  stmt->type = type;
  stmt->expr = expr;
  stmt->name = name;
  return stmt;
};

Statement* statement(Parser* parser) {
  if (match(parser, (TokenType[]){PRINT, -1}))
    return print_statement(parser);
  return expression_statement(parser);
};

Statement* print_statement(Parser* parser) {
  Expr* expr = expression(parser);
  consume(parser, SEMICOLON, "Expect ';' after value.");
  return new_statement(STATEMENT_PRINT, expr, NULL);
};

Statement* expression_statement(Parser* parser) {
  Expr* expr = expression(parser);
  consume(parser, SEMICOLON, "Expect ';' after expression.");
  return new_statement(STATEMENT_EXPRESSION, expr, NULL);
};

Expr* expression(Parser* parser) {
  return equality(parser);
};

Expr* equality(Parser* parser) {
  Expr* expr = comparison(parser);
  while (match(parser, (TokenType[]){BANG_EQUAL, EQUAL_EQUAL, -1})) {
    Token* op = previous(parser);
    Expr* right = comparison(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* comparison(Parser* parser) {
  Expr* expr = term(parser);
  while (match(parser,
               (TokenType[]){GREATER, GREATER_EQUAL, LESS, LESS_EQUAL, -1})) {
    Token* op = previous((parser));
    Expr* right = term(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* term(Parser* parser) {
  Expr* expr = factor(parser);
  while (match(parser, (TokenType[]){MINUS, PLUS, -1})) {
    Token* op = previous((parser));
    Expr* right = factor(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* factor(Parser* parser) {
  Expr* expr = unary(parser);
  while (match(parser, (TokenType[]){SLASH, STAR, -1})) {
    Token* op = previous((parser));
    Expr* right = unary(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* unary(Parser* parser) {
  if (match(parser, (TokenType[]){BANG, MINUS, -1})) {
    Token* op = previous(parser);
    Expr* right = unary(parser);
    return new_unary(op, right);
  }
  return primary(parser);
};

Expr* primary(Parser* parser) {
  if (match(parser, (TokenType[]){FALSE, TRUE, NIL, STRING, NUMBER, -1})) {
    return new_literal(previous(parser));
  }

  if (match(parser, (TokenType[]){IDENTIFIER})) {
    return new_variable(previous(parser));
  }

  // matched group
  if (match(parser, (TokenType[]){LEFT_PAREN, -1})) {
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

bool match(Parser* parser, TokenType* types) {
  if (is_at_end((parser))) {
    return false;
  }
  for (int i = 0; types[i] != -1; i++) {
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

void print_ast(Statement** statements) {
  for (int i = 0; statements[i] != NULL; i++) {
    Statement* stmt = statements[i];
    print_expr(stmt->expr);
  }
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