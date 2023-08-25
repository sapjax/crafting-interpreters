
#include <stdio.h>
#include <stdlib.h>
#include "include/interpreter.h"
#include "include/lexer.h"
#include "include/parser.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: %s <source>\n", argv[0]);
    return 1;
  }
  char* path = argv[1];
  FILE* file = fopen(path, "r");
  if (file == NULL) {
    printf("Error: Could not open file %s\n", path);
    return 1;
  }
  fseek(file, 0, SEEK_END);
  size_t length = ftell(file);
  fseek(file, 0, SEEK_SET);
  char* source = malloc(length + 1);
  fread(source, 1, length, file);
  fclose(file);
  source[length] = '\0';

  Lexer* lexer = new_lexer(source);
  Token** tokens = scan_tokens(lexer);
  print_lexer(lexer);
  Parser* parser = new_parser(tokens, lexer->num_tokens);
  Expr* expression = parse(parser);
  print_ast(expression);
  interpret(expression);
  return 0;
}
