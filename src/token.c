#include "include/token.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token* new_token(TokenType type,
                 const char* lexeme,
                 Literal* literal,
                 int line) {
  Token* token = (Token*)malloc(sizeof(Token));
  if (token == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    return NULL;
  }

  token->type = type;
  // Use strdup to duplicate the lexeme string
  token->lexeme = lexeme != NULL ? strdup(lexeme) : NULL;
  token->literal = literal != NULL ? literal : NULL;
  token->line = line;
  return token;
}

void print_token(Token* token) {
  printf("%s", type_to_string(token->type));
  if (token->lexeme != NULL) {
    printf(" %s", token->lexeme);
  }
  printf("\n");
};

TokenType map_keyword(char* text) {
  struct {
    char* keyword;
    TokenType type;
  } keywords[] = {
      {"and", AND},   {"class", CLASS}, {"else", ELSE},     {"false", FALSE},
      {"for", FOR},   {"fun", FUN},     {"if", IF},         {"nil", NIL},
      {"or", OR},     {"print", PRINT}, {"return", RETURN}, {"super", SUPER},
      {"this", THIS}, {"true", TRUE},   {"var", VAR},       {"while", WHILE},
  };
  int num_keywords = sizeof(keywords) / sizeof(keywords[0]);
  for (int i = 0; i < num_keywords; i++) {
    if (strcmp(text, keywords[i].keyword) == 0) {
      return keywords[i].type;
    }
  }
  return -1;
};

char* type_to_string(TokenType type) {
  switch (type) {
    case LEFT_PAREN:
      return "(";
    case RIGHT_PAREN:
      return ")";
    case LEFT_BRACE:
      return "{";
    case RIGHT_BRACE:
      return "}";
    case COMMA:
      return ",";
    case DOT:
      return ".";
    case MINUS:
      return "-";
    case PLUS:
      return "+";
    case SEMICOLON:
      return ";";
    case SLASH:
      return "/";
    case STAR:
      return "*";
    case BANG:
      return "!";
    case BANG_EQUAL:
      return "!=";
    case EQUAL:
      return "=";
    case EQUAL_EQUAL:
      return "==";
    case GREATER:
      return ">";
    case GREATER_EQUAL:
      return ">=";
    case LESS:
      return "";
    case LESS_EQUAL:
      return "<=";
    case IDENTIFIER:
      return "IDENTIFIER";
    case STRING:
      return "STRING";
    case NUMBER:
      return "NUMBER";
    case AND:
      return "AND";
    case CLASS:
      return "CLASS";
    case ELSE:
      return "ELSE";
    case FALSE:
      return "FALSE";
    case FUN:
      return "FUN";
    case FOR:
      return "FOR";
    case IF:
      return "IF";
    case NIL:
      return "NIL";
    case OR:
      return "OR";
    case PRINT:
      return "PRINT";
    case RETURN:
      return "RETURN";
    case SUPER:
      return "SUPER";
    case THIS:
      return "THIS";
    case TRUE:
      return "TRUE";
    case VAR:
      return "VAR";
    case WHILE:
      return "WHILE";
    case E_O_F:
      return "EOF";
    default:
      return "UNKNOWN";
  }
}