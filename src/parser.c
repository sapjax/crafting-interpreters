#include "include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Statement* statement(Parser* parser);
Statement* statement_print(Parser* parser);
Statement* statement_expression(Parser* parser);
Statement* statement_block(Parser* parser);
Statement* declare_var(Parser* parser);
Statement* declare_class(Parser* parser);
Statement* declare_fun(Parser* parser, char* kind);
Statement* statement_if(Parser* parser);
Statement* statement_while(Parser* parser);
Statement* statement_for(Parser* parser);
Statement* statement_return(Parser* parser);
Statement* new_statement(StatementType type);

Statement* declaration(Parser* parser);

Expr* expression(Parser* parser);
Expr* assignment(Parser* parser);
Expr* logic_or(Parser* parser);
Expr* logci_and(Parser* parser);
Expr* equality(Parser* parser);
Expr* comparison(Parser* parser);
Expr* term(Parser* parser);
Expr* factor(Parser* parser);
Expr* unary(Parser* parser);
Expr* call(Parser* parser);
Expr* finish_call(Parser* parser, Expr* expr);
Expr* primary(Parser* parser);

static Token* advance(Parser* parser);
static Token* peek(Parser* parser);
static Token* previous(Parser* parser);
static Token* consume(Parser* parser, TokenType type, char* message);

static void error(Token* token, char* message);
static void synchronize(Parser* parser);

static bool match(Parser* parser, TokenType type);
static bool match_any(Parser* parser, TokenType* types);
static bool check(Parser* parser, TokenType type);
static bool is_at_end(Parser* parser);

Expr* new_expr(UnTaggedExpr* u_expr, ExprType type);
Expr* new_binary(Expr* left, Token* op, Expr* right);
Expr* new_unary(Token* op, Expr* right);
Expr* new_literal(Token* value);
Expr* new_call(Expr* callee, Token* paren, Expr** arguments);
Expr* new_grouping(Expr* expression);
Expr* new_get(Expr* object, Token* name);
Expr* new_set(Expr* object, Token* name, Expr* value);
Expr* new_this(Token* keyword);
Expr* new_variable(Token* value);
Expr* new_assign(Token* name, Expr* value);
Expr* new_logical(Expr* left, Token* op, Expr* right);

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

Expr* new_call(Expr* callee, Token* paren, Expr** arguments) {
  ExprCall* call = malloc(sizeof(ExprCall));
  call->callee = callee;
  call->paren = paren;
  call->arguments = arguments;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->call = call;
  return new_expr(u_expr, E_Call);
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
  variable->depth = -1;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->variable = variable;
  return new_expr(u_expr, E_Variable);
};

Expr* new_get(Expr* object, Token* name) {
  ExprGet* get = malloc(sizeof(ExprGet));
  get->object = object;
  get->name = name;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->get = get;
  return new_expr(u_expr, E_Get);
};

Expr* new_set(Expr* object, Token* name, Expr* value) {
  ExprSet* set = malloc(sizeof(ExprSet));
  set->object = object;
  set->name = name;
  set->value = value;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->set = set;
  return new_expr(u_expr, E_Set);
};

Expr* new_this(Token* keyword) {
  ExprThis* this = malloc(sizeof(ExprThis));
  this->keyword = keyword;
  this->keyword->lexeme = "this";
  this->depth = -1;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->this = this;
  return new_expr(u_expr, E_This);
};

Expr* new_assign(Token* name, Expr* value) {
  ExprAssign* assign = malloc(sizeof(ExprAssign));
  assign->name = name;
  assign->value = value;
  assign->depth = -1;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->assign = assign;
  return new_expr(u_expr, E_Assign);
};

Expr* new_logical(Expr* left, Token* op, Expr* right) {
  ExprLogical* logical = malloc(sizeof(ExprLogical));
  logical->left = left;
  logical->op = op;
  logical->right = right;
  UnTaggedExpr* u_expr = new_untagged_expr();
  u_expr->logical = logical;
  return new_expr(u_expr, E_Logical);
};

Statement** parse(Parser* parser) {
  Statement** stmts = malloc(sizeof(Statement*) * 0 + sizeof(NULL));
  int i = 0;
  while (!is_at_end(parser)) {
    stmts = realloc(stmts, sizeof(Statement*) * (i + 1) + sizeof(NULL));
    stmts[i] = declaration(parser);
    i++;
  }
  // add NULL at end for loop stop flag.
  stmts[i] = NULL;
  return stmts;
};

Statement* declaration(Parser* parser) {
  if (match(parser, VAR))
    return declare_var(parser);
  if (match(parser, CLASS))
    return declare_class(parser);
  if (match(parser, FUN))
    return declare_fun(parser, "function");
  return statement(parser);
};

Statement* new_statement(StatementType type) {
  Statement* stmt = malloc(sizeof(Statement));
  stmt->type = type;
  UnTaggedStatement* u_stmt = malloc(sizeof(UnTaggedStatement));
  stmt->u_stmt = u_stmt;
  switch (type) {
    case STATEMENT_VAR:
      u_stmt->var = malloc(sizeof(StatementVar));
      break;
    case STATEMENT_PRINT:
      u_stmt->print = malloc(sizeof(StatementPrint));
      break;
    case STATEMENT_EXPRESSION:
      u_stmt->expr = malloc(sizeof(StatementExpression));
      break;
    case STATEMENT_BLOCK:
      u_stmt->block = malloc(sizeof(StatementBlock));
      break;
    case STATEMENT_IF:
      u_stmt->if_stmt = malloc(sizeof(StatementIf));
      break;
    case STATEMENT_WHILE:
      u_stmt->while_stmt = malloc(sizeof(StatementWhile));
      break;
    case STATEMENT_FUNCTION:
      u_stmt->function = malloc(sizeof(StatementFunction));
      break;
    case STATEMENT_CLASS:
      u_stmt->class = malloc(sizeof(StatementClass));
      break;
    case STATEMENT_RETURN:
      u_stmt->return_stmt = malloc(sizeof(StatementReturn));
      break;
    default:
      break;
  }
  return stmt;
};

Statement* statement(Parser* parser) {
  if (match(parser, IF))
    return statement_if(parser);
  if (match(parser, PRINT))
    return statement_print(parser);
  if (match(parser, RETURN))
    return statement_return(parser);
  if (match(parser, WHILE))
    return statement_while(parser);
  if (match(parser, FOR))
    return statement_for(parser);
  if (match(parser, LEFT_BRACE))
    return statement_block(parser);
  return statement_expression(parser);
};

Statement* declare_var(Parser* parser) {
  Token* name = consume(parser, IDENTIFIER, "Expect variable name.");
  Expr* initializer = NULL;
  if (match(parser, EQUAL)) {
    initializer = expression(parser);
  }
  consume(parser, SEMICOLON, "Expect ';' after variable declaration.");
  Statement* stmt = new_statement(STATEMENT_VAR);
  stmt->u_stmt->var->name = name;
  stmt->u_stmt->var->initializer = initializer;
  return stmt;
};

Statement* declare_class(Parser* parser) {
  Token* name = consume(parser, IDENTIFIER, "Expect class name.");
  consume(parser, LEFT_BRACE, "Expect '{' before class body.");
  Statement** methods = malloc(sizeof(Statement*) * 0 + sizeof(NULL));
  int i = 0;
  while (!check(parser, RIGHT_BRACE) && !is_at_end(parser)) {
    methods = realloc(methods, sizeof(Statement*) * (i + 1) + sizeof(NULL));
    methods[i] = declare_fun(parser, "method");
    i++;
  }
  methods[i] = NULL;
  consume(parser, RIGHT_BRACE, "Expect '}' after class body.");
  Statement* stmt = new_statement(STATEMENT_CLASS);
  stmt->u_stmt->class->name = name;
  stmt->u_stmt->class->methods = methods;
  return stmt;
};

Statement* declare_fun(Parser* parser, char* kind) {
  bool is_method = strcmp(kind, "method") == 0;
  Token* name =
      consume(parser, IDENTIFIER,
              is_method ? "Expect method name." : "Expect function name.");
  consume(parser, LEFT_PAREN,
          is_method ? "Expect '(' after method name."
                    : "Expect '(' after function name.");
  Token** parameters = malloc(sizeof(Token*) * 0 + sizeof(NULL));
  int i = 0;
  if (!check(parser, RIGHT_PAREN)) {
    do {
      if (i >= 255) {
        error(peek(parser), "Can't have more than 255 parameters.");
      }
      parameters = realloc(parameters, sizeof(Token*) * (i + 1) + sizeof(NULL));
      parameters[i] = consume(parser, IDENTIFIER, "Expect parameter name.");
      i++;
    } while (match(parser, COMMA));
  }
  parameters[i] = NULL;
  consume(parser, RIGHT_PAREN, "Expect ')' after parameters.");

  consume(parser, LEFT_BRACE,
          is_method ? "Expect '{' before method body."
                    : "Expect '{' before function body.");

  Statement* body = statement_block(parser);
  Statement* stmt = new_statement(STATEMENT_FUNCTION);
  stmt->u_stmt->function->name = name;
  stmt->u_stmt->function->params = parameters;
  stmt->u_stmt->function->body = body;
  return stmt;
};

Statement* statement_if(Parser* parser) {
  consume(parser, LEFT_PAREN, "Expect '(' after 'if'.");
  Expr* condition = expression(parser);
  consume(parser, RIGHT_PAREN, "Expect ')' after if condition.");
  Statement* then_branch = statement(parser);
  Statement* else_branch = NULL;
  if (match(parser, ELSE)) {
    else_branch = statement(parser);
  }
  Statement* stmt = new_statement(STATEMENT_IF);
  stmt->u_stmt->if_stmt->condition = condition;
  stmt->u_stmt->if_stmt->then_branch = then_branch;
  stmt->u_stmt->if_stmt->else_branch = else_branch;
  return stmt;
};

Statement* statement_print(Parser* parser) {
  Expr* expr = expression(parser);
  consume(parser, SEMICOLON, "Expect ';' after value.");
  Statement* stmt = new_statement(STATEMENT_PRINT);
  stmt->u_stmt->print->expr = expr;
  return stmt;
};

Statement* statement_return(Parser* parser) {
  Token* keyword = previous(parser);
  Expr* value = NULL;
  if (!check(parser, SEMICOLON)) {
    value = expression(parser);
  }
  consume(parser, SEMICOLON, "Expect ';' after return value.");
  Statement* stmt = new_statement(STATEMENT_RETURN);
  stmt->u_stmt->return_stmt->keyword = keyword;
  stmt->u_stmt->return_stmt->value = value;
  return stmt;
};

Statement* statement_while(Parser* parser) {
  consume(parser, LEFT_PAREN, "Expect '(' after 'while'.");
  Expr* condition = expression(parser);
  consume(parser, RIGHT_PAREN, "Expect ')' after condition.");
  Statement* body = statement(parser);
  Statement* stmt = new_statement(STATEMENT_WHILE);
  stmt->u_stmt->while_stmt->condition = condition;
  stmt->u_stmt->while_stmt->body = body;
  return stmt;
};

Statement* statement_for(Parser* parser) {
  consume(parser, LEFT_PAREN, "Expect '(' after 'for'.");

  Statement* initializer;
  if (match(parser, SEMICOLON)) {
    initializer = NULL;
  } else if (match(parser, VAR)) {
    initializer = declare_var(parser);
  } else {
    initializer = statement_expression(parser);
  }

  Expr* condition = NULL;
  if (!check(parser, SEMICOLON)) {
    condition = expression(parser);
  }
  consume(parser, SEMICOLON, "Expect ';' after loop condition.");

  Expr* increment = NULL;
  if (!check(parser, RIGHT_PAREN)) {
    increment = expression(parser);
  }
  consume(parser, RIGHT_PAREN, "Expect ')' after for clauses.");

  Statement* body = statement(parser);

  if (increment != NULL) {
    Statement* increment_stmt = new_statement(STATEMENT_EXPRESSION);
    increment_stmt->u_stmt->expr->expr = increment;

    Statement* block = new_statement(STATEMENT_BLOCK);
    block->u_stmt->block->stmts = (Statement**)malloc(sizeof(Statement*) * 3);
    block->u_stmt->block->stmts[0] = body;
    block->u_stmt->block->stmts[1] = increment_stmt;
    block->u_stmt->block->stmts[2] = NULL;
    body = block;
  }

  if (condition == NULL)
    condition = new_literal(new_token(TRUE, NULL, NULL, 0));

  Statement* while_stmt = new_statement(STATEMENT_WHILE);
  while_stmt->u_stmt->while_stmt->condition = condition;
  while_stmt->u_stmt->while_stmt->body = body;
  body = while_stmt;

  if (initializer != NULL) {
    Statement* block = new_statement(STATEMENT_BLOCK);
    block->u_stmt->block->stmts = (Statement**)malloc(sizeof(Statement*) * 3);
    block->u_stmt->block->stmts[0] = initializer;
    block->u_stmt->block->stmts[1] = body;
    block->u_stmt->block->stmts[2] = NULL;
    body = block;
  }

  return body;
};

Statement* statement_expression(Parser* parser) {
  Expr* expr = expression(parser);
  consume(parser, SEMICOLON, "Expect ';' after expression.");
  Statement* stmt = new_statement(STATEMENT_EXPRESSION);
  stmt->u_stmt->expr->expr = expr;
  return stmt;
};

Statement* statement_block(Parser* parser) {
  Statement** stmts = calloc(1, sizeof(Statement*) + sizeof(NULL));
  int i = 0;
  while (!check(parser, RIGHT_BRACE) && !is_at_end(parser)) {
    stmts = realloc(stmts, sizeof(Statement*) * (i + 1) + sizeof(NULL));
    stmts[i] = declaration(parser);
    i++;
  }
  stmts[i] = NULL;
  consume(parser, RIGHT_BRACE, "Expect '}' after block.");
  Statement* stmt = new_statement(STATEMENT_BLOCK);
  stmt->u_stmt->block->stmts = stmts;
  return stmt;
};

Expr* expression(Parser* parser) {
  return assignment(parser);
};

Expr* assignment(Parser* parser) {
  Expr* expr = logic_or(parser);
  if (match(parser, EQUAL)) {
    Token* equals = previous(parser);
    Expr* value = assignment(parser);

    if (expr->type == E_Variable) {
      Token* name = expr->u_expr->variable->name;
      return new_assign(name, value);
    } else if (expr->type == E_Get) {
      ExprGet* get = expr->u_expr->get;
      return new_set(get->object, get->name, value);
    }

    free(value);
    error(equals, "Invalid assignment target.");
  }
  return expr;
}

Expr* logic_or(Parser* parser) {
  Expr* expr = logci_and(parser);
  while (match(parser, OR)) {
    Token* op = previous(parser);
    Expr* right = logci_and(parser);
    expr = new_logical(expr, op, right);
  }
  return expr;
}

Expr* logci_and(Parser* parser) {
  Expr* expr = equality(parser);
  while (match(parser, AND)) {
    Token* op = previous(parser);
    Expr* right = equality(parser);
    expr = new_logical(expr, op, right);
  }
  return expr;
}

Expr* equality(Parser* parser) {
  Expr* expr = comparison(parser);
  while (match_any(parser, (TokenType[]){BANG_EQUAL, EQUAL_EQUAL, -1})) {
    Token* op = previous(parser);
    Expr* right = comparison(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* comparison(Parser* parser) {
  Expr* expr = term(parser);
  while (match_any(
      parser, (TokenType[]){GREATER, GREATER_EQUAL, LESS, LESS_EQUAL, -1})) {
    Token* op = previous((parser));
    Expr* right = term(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* term(Parser* parser) {
  Expr* expr = factor(parser);
  while (match_any(parser, (TokenType[]){MINUS, PLUS, -1})) {
    Token* op = previous((parser));
    Expr* right = factor(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* factor(Parser* parser) {
  Expr* expr = unary(parser);
  while (match_any(parser, (TokenType[]){SLASH, STAR, -1})) {
    Token* op = previous((parser));
    Expr* right = unary(parser);
    Expr* binary = new_binary(expr, op, right);
    expr = binary;
  }
  return expr;
};

Expr* unary(Parser* parser) {
  if (match_any(parser, (TokenType[]){BANG, MINUS, -1})) {
    Token* op = previous(parser);
    Expr* right = unary(parser);
    return new_unary(op, right);
  }
  return call(parser);
};

Expr* call(Parser* parser) {
  Expr* expr = primary(parser);

  while (true) {
    if (match(parser, LEFT_PAREN)) {
      expr = finish_call(parser, expr);
    } else if (match(parser, DOT)) {
      Token* name =
          consume(parser, IDENTIFIER, "Expect property name after '.'.");
      expr = new_get(expr, name);

    } else {
      break;
    }
  }
  return expr;
};

Expr* finish_call(Parser* parser, Expr* callee) {
  Expr** arguments = malloc(sizeof(Expr*) * 0 + sizeof(NULL));
  int i = 0;
  if (!check(parser, RIGHT_PAREN)) {
    do {
      arguments = realloc(arguments, sizeof(Expr*) * (i + 1) + sizeof(NULL));
      arguments[i] = expression(parser);
      i++;
    } while (match(parser, COMMA));
    arguments[i] = NULL;
  }

  Token* paren = consume(parser, RIGHT_PAREN, "Expect ')' after arguments.");
  return new_call(callee, paren, arguments);
};

Expr* primary(Parser* parser) {
  if (match_any(parser, (TokenType[]){FALSE, TRUE, NIL, STRING, NUMBER, -1})) {
    return new_literal(previous(parser));
  }

  if (match(parser, THIS)) {
    return new_this(previous(parser));
  }

  if (match(parser, IDENTIFIER)) {
    return new_variable(previous(parser));
  }

  // matched group
  if (match(parser, LEFT_PAREN)) {
    Expr* expr = expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after expression.");
    return new_grouping(expr);
  }

  if (match(parser, LEFT_BRACE)) {
    Statement* stmts = statement_block(parser);
    free(stmts);
    return NULL;
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

bool match(Parser* parser, TokenType type) {
  if (is_at_end((parser))) {
    return false;
  }
  if (check(parser, type)) {
    advance(parser);
    return true;
  }
  return false;
};

bool match_any(Parser* parser, TokenType* types) {
  if (is_at_end((parser))) {
    return false;
  }
  for (int i = 0; types[i] != (TokenType)-1; i++) {
    if (check(parser, types[i])) {
      advance(parser);
      return true;
    }
  }
  return false;
};
