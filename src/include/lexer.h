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

void print_lexer(Lexer* lexer);

Token** scan_tokens(Lexer* lexer);

#endif  // LOX_LEXER_H