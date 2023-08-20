#include "include/lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/token.h"

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

Lexer* new_lexer(char* source) {
  Lexer* lexer = calloc(1, sizeof(Lexer));
  lexer->source = source;
  lexer->start = 0;
  lexer->current = 0;
  lexer->line = 0;
  lexer->num_tokens = 0;
  lexer->tokens = calloc(1, sizeof(Token*));
  return lexer;
};

void print_lexer(Lexer* lexer) {
  for (int i = 0; i < lexer->num_tokens; i++) {
    Token* token = lexer->tokens[i];
    print_token(token);
  }
}

Token** scan_tokens(Lexer* lexer) {
  while (!is_at_end(lexer)) {
    lexer->start = lexer->current;
    scan_token(lexer);
  };
  add_token(lexer, E_O_F);
  return lexer->tokens;
}

void scan_token(Lexer* lexer) {
  char c = advance(lexer);
  switch (c) {
    case '(':
      add_token(lexer, LEFT_PAREN);
      break;
    case ')':
      add_token(lexer, RIGHT_PAREN);
      break;
    case '{':
      add_token(lexer, LEFT_BRACE);
      break;
    case '}':
      add_token(lexer, RIGHT_BRACE);
      break;
    case ',':
      add_token(lexer, COMMA);
      break;
    case '-':
      add_token(lexer, MINUS);
      break;
    case '+':
      add_token(lexer, PLUS);
      break;
    case ';':
      add_token(lexer, SEMICOLON);
      break;
    case '*':
      add_token(lexer, STAR);
      break;
    // look second char
    case '!':
      add_token(lexer, match(lexer, '=') ? BANG_EQUAL : BANG);
      break;
    case '=':
      add_token(lexer, match(lexer, '=') ? EQUAL_EQUAL : EQUAL);
      break;
    case '<':
      add_token(lexer, match(lexer, '=') ? LESS_EQUAL : LESS);
      break;
    case '>':
      add_token(lexer, match(lexer, '=') ? GREATER_EQUAL : GREATER);
      break;
    // handle comment
    case '/':
      if (match(lexer, '/')) {
        // A comment goes until the end of line.
        while (peek(lexer) != '\n' && !is_at_end(lexer))
          advance(lexer);
      } else {
        add_token(lexer, SLASH);
      }
      // skip whitespace
    case ' ':
    case '\r':
    case '\t':
      break;
    case '\n':
      lexer->line++;
      break;
    case '"':
      parse_string(lexer);
      break;
    default:
      if (is_digit(c)) {
        parse_number(lexer);
      } else if (is_alpha(c)) {
        parse_identifier(lexer);
      } else {
        fprintf(stderr, "Lexer Error: Unexpected character at %d.\n",
                lexer->line);
        exit(EXIT_FAILURE);
      }
      break;
  }
}

char advance(Lexer* lexer) {
  if (!is_at_end(lexer)) {
    return lexer->source[lexer->current++];
  }
  return '\0';
}

void add_token(Lexer* lexer, TokenType type) {
  _add_token(lexer, new_token(type, NULL, NULL, lexer->line));
}

void add_token_with_literal(Lexer* lexer,
                            TokenType type,
                            char* lexeme,
                            Literal* literal) {
  _add_token(lexer, new_token(type, lexeme, literal, lexer->line));
  free(lexeme);
};

void _add_token(Lexer* lexer, Token* token) {
  int len = lexer->num_tokens;
  lexer->tokens = realloc(lexer->tokens, sizeof(Token*) * (len + 1));
  if (lexer->tokens == NULL) {
    fprintf(stderr, "Lexer Error: Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }
  lexer->num_tokens = len + 1;
  lexer->tokens[len] = token;
}

bool is_at_end(Lexer* lexer) {
  return lexer->current >= (int)strlen(lexer->source) ||
         lexer->source[lexer->current] == '\0';
};

bool match(Lexer* lexer, char expected) {
  if (is_at_end(lexer))
    return false;
  if (lexer->source[lexer->current] != expected)
    return false;
  lexer->current++;
  return true;
};

char peek(Lexer* lexer) {
  if (is_at_end(lexer))
    return '\0';
  return lexer->source[lexer->current];
};

char peek_next(Lexer* lexer) {
  if (lexer->current + 1 >= (int)strlen(lexer->source))
    return '\0';
  return lexer->source[lexer->current + 1];
};

void parse_string(Lexer* lexer) {
  while (peek(lexer) != '"' && !is_at_end(lexer)) {
    if (peek(lexer) == '\n')
      lexer->line++;
    advance(lexer);
  }

  if (is_at_end(lexer)) {
    fprintf(stderr, "Lexer Error: Unterminated string at %d.\n", lexer->line);
    exit(EXIT_FAILURE);
    return;
  }

  // The closing ".
  advance(lexer);

  // Trim the surrounding quotes.
  char* lexeme = substr(lexer->source, lexer->start + 1, lexer->current - 1);

  Literal* literal = (Literal*)malloc(sizeof(Literal));
  literal->string = strdup(lexeme);
  add_token_with_literal(lexer, STRING, lexeme, literal);
};

void parse_number(Lexer* lexer) {
  while (is_digit(peek(lexer)))
    advance(lexer);

  // Look for a fractional part.
  if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
    // Consume the "."
    advance(lexer);

    while (is_digit(peek(lexer)))
      advance(lexer);
  }

  char* lexeme = substr(lexer->source, lexer->start, lexer->current);
  double n = atof(lexeme);
  Literal* literal = (Literal*)malloc(sizeof(Literal));
  literal->number = n;
  add_token_with_literal(lexer, NUMBER, lexeme, literal);
};

void parse_identifier(Lexer* lexer) {
  while (isalnum((unsigned char)peek(lexer)))
    advance(lexer);
  char* lexeme = substr(lexer->source, lexer->start, lexer->current);
  TokenType type = map_keyword(lexeme);
  if (type != (TokenType)-1) {  // keywords
    add_token(lexer, type);
  } else {  // identifier
    Literal* literal = (Literal*)malloc(sizeof(Literal));
    literal->identifier = strdup(lexeme);
    add_token_with_literal(lexer, IDENTIFIER, lexeme, literal);
  }
};

char* substr(const char* src, int start, int end) {
  int len = end - start;
  char* dest = (char*)malloc(
      (len + 1) * sizeof(char));  // Allocate memory for the destination string
  if (dest == NULL) {
    fprintf(stderr, "Lexer Error: Memory allocation failed.\n");
    return NULL;
  }

  strncpy(dest, src + start, len);
  dest[len] = '\0';
  return dest;
}

bool is_digit(char c) {
  return c >= '0' && c <= '9';
};

bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
};
