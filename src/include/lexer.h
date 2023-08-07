#ifndef LOX_LEXER_H
#define LOX_LEXER_H
#include <stdbool.h>
#include "token.h"

typedef struct Lexer {
  char* source;
  int start;
  int current;
  int line;
  Token** tokens;
  int num_tokens;
} Lexer;

Lexer* new_lexer(char* source);

void print_lexer();

Token** scan_tokens(Lexer* lexer);

void scan_token(Lexer* lexer);

char advance(Lexer* lexer);

void add_token(Lexer* lexer, TokenType type);

void add_token_with_literal(Lexer* lexer,
                            TokenType type,
                            char* lexeme,
                            Literal* literal);

void _add_token(Lexer* lexer, Token* token);

bool is_at_end(Lexer* lexer);

bool match(Lexer* lexer, char expected);

char peek(Lexer* lexer);

char peek_next(Lexer* lexer);

void parse_string(Lexer* lexer);

bool is_digit(char c);

void parse_number(Lexer* lexer);

void parse_identifier(Lexer* lexer);

char* substr(const char* src, int start, int end);

bool is_alpha(char c);

#endif  // LOX_LEXER_H