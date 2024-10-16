#include "lexer.h"

#include <cassert>

#define VERBOSE

void assert_and_print_error(Lexer *lexer, Token *left, Token *right) {
  if (!token_equals(left, right)) {
    printf("Lexer beginning of current token: %c\n",
           *lexer->beginning_of_current_token);
    printf("Lexer current position: %c\n", *lexer->current_location);
    printf("Lexer char after current position: %c\n",
           lexer->current_location[1]);

    printf("Token left value: %d\n", (int)left->type);
    printf("Token left string: %s\n", left->string.c_str());
    printf("Token right value: %d\n", (int)right->type);
    printf("Token right string: %s\n", right->string.c_str());
    assert(token_equals(left, right));
  }
#ifdef VERBOSE
  else {
    printf("successfully matched token starting with %c and having string %s\n",
           *lexer->beginning_of_current_token, left->string.c_str());
    printf("Lexer's current character is %c\n", *lexer->current_location);
  }
#endif
}

Token int_token = make_token(TokenType::Int, 0, 0);
Token x_token = make_token(TokenType::Identifier, 0, 0, "x");
Token equal_token = make_token(TokenType::Equals, 0, 0);
Token five_token = make_token(TokenType::Number, 0, 0, "5");
Token semicolon_token = make_token(TokenType::Semicolon, 0, 0);
Token eof_token = make_token(TokenType::Eof, 0, 0);

void test1() {
  printf("running lexer test 1...\n");

  const char *test1 = "int x = 5;";
  Lexer lexer1 = new_lexer(test1);

  assert_and_print_error(&lexer1, get_next_token(&lexer1), &int_token);
  assert_and_print_error(&lexer1, get_next_token(&lexer1), &x_token);
  assert_and_print_error(&lexer1, get_next_token(&lexer1), &equal_token);
  assert_and_print_error(&lexer1, get_next_token(&lexer1), &five_token);
  assert_and_print_error(&lexer1, get_next_token(&lexer1), &semicolon_token);
  assert_and_print_error(&lexer1, get_next_token(&lexer1), &eof_token);
  printf("Lexer test 1 passed\n\n");
}

void test2() {
  printf("running lexer test 2...\n");

  const char *test2 = "int x=5;";
  Lexer lexer2 = new_lexer(test2);

  assert_and_print_error(&lexer2, get_next_token(&lexer2), &int_token);
  assert_and_print_error(&lexer2, get_next_token(&lexer2), &x_token);
  assert_and_print_error(&lexer2, get_next_token(&lexer2), &equal_token);
  assert_and_print_error(&lexer2, get_next_token(&lexer2), &five_token);
  assert_and_print_error(&lexer2, get_next_token(&lexer2), &semicolon_token);
  assert_and_print_error(&lexer2, get_next_token(&lexer2), &eof_token);

  printf("Lexer test 2 passed\n\n");
}

int main() {
  printf("running lexer tests...\n");

  test1();
  test2();
}
