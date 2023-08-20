#ifndef LOX_TOKEN_H
#define LOX_TOKEN_H

typedef union {
  char* string;
  char* identifier;
  double number;
} Literal;

typedef enum TokenType {
  // single-character tokens
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  COMMA,
  DOT,
  MINUS,
  PLUS,
  SEMICOLON,
  SLASH,
  STAR,
  // one or two character tokens
  BANG,
  BANG_EQUAL,
  EQUAL,
  EQUAL_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,
  // literals
  IDENTIFIER,
  STRING,
  NUMBER,
  // keywords
  AND,
  CLASS,
  ELSE,
  FALSE,
  FUN,
  FOR,
  IF,
  NIL,
  OR,
  PRINT,
  RETURN,
  SUPER,
  THIS,
  TRUE,
  VAR,
  WHILE,
  // EOF
  E_O_F,
} TokenType;

typedef struct Token {
  TokenType type;
  char* lexeme;
  Literal* literal;
  int line;
} Token;

Token* new_token(TokenType type, char* lexeme, Literal* literal, int line);

TokenType map_keyword(char* text);

void print_token(Token* token);

char* type_to_string(TokenType type);

#endif  // LOX_TOKEN_H