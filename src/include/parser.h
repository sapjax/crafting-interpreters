#ifndef LOX_PARSER_H
#define LOX_PARSER_H
#include "expression.h"

typedef struct Parser {
  Token** tokens;
  int current;
} Parser;

/** rules of parser
 *
        expression     → equality ;
        equality       → comparison ( ( "!=" | "==" ) comparison )* ;
        comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
        term           → factor ( ( "-" | "+" ) factor )* ;
        factor         → unary ( ( "/" | "*" ) unary )* ;
        unary          → ( "!" | "-" ) unary
                                | primary ;
        primary        → "true" | "false" | "nil" | "this"
                          | NUMBER | STRING | IDENTIFIER | "(" expression ")"
                          | "super" "." IDENTIFIER ;
*/

Parser* new_parser(Token** tokens, int num_tokens);

Statement** parse(Parser* parser);

#endif