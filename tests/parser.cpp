#include "parser.h"
#include "lexer.h"
#include <cassert>

void test1() {
  printf("Running parser test 1...\n");

  const char *source = "1";
  Lexer lexer = new_lexer(source);

  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Number);

  ASTNode *node = parse_expression(&lexer);
  assert(node->data_as.int_data == 1);
  assert(node->lhs == nullptr);
  assert(node->rhs == nullptr);
  assert(get_current_token(&lexer)->type == TokenType::Eof);
  assert(*lexer.current_location == '\0');

  printf("test 1 passed\n\n");
}

void test2() {
  printf("Running parser test 2...\n");

  const char *source = "20";
  Lexer lexer = new_lexer(source);
  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Number);

  ASTNode *node = parse_expression(&lexer);
  assert(node->data_as.int_data == 20);
  assert(node->lhs == nullptr);
  assert(node->rhs == nullptr);
  assert(get_current_token(&lexer)->type == TokenType::Eof);
  assert(*lexer.current_location == '\0');

  printf("test 2 passed\n\n");
}

void test3() {
  printf("Running parser test 3...\n");

  const char *source = "20 * 6";
  Lexer lexer = new_lexer(source);
  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Number);

  ASTNode *node = parse_expression(&lexer);
  assert(node->type == ASTNodeType::Multiplication);
  assert(node->lhs->data_as.int_data == 20);
  assert(node->rhs->data_as.int_data == 6);
  assert(get_current_token(&lexer)->type == TokenType::Eof);
  assert(*lexer.current_location == '\0');

  printf("test 3 passed\n\n");
}

void test4() {
  printf("Running parser test 4...\n");

  const char *source = "20 * 6123 / 330 % 2";
  Lexer lexer = new_lexer(source);
  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Number);

  ASTNode *node = parse_expression(&lexer);

  assert(node->type == ASTNodeType::Modulo);
  assert(node->rhs->data_as.int_data == 2);

  ASTNode *div_node = node->lhs;
  assert(div_node->type == ASTNodeType::Division);
  assert(div_node->rhs->data_as.int_data == 330);

  ASTNode *mod_node = div_node->lhs;
  assert(mod_node->type == ASTNodeType::Multiplication);
  assert(mod_node->lhs->data_as.int_data == 20);
  assert(mod_node->rhs->data_as.int_data == 6123);

  assert(get_current_token(&lexer)->type == TokenType::Eof);
  assert(*lexer.current_location == '\0');

  printf("test 4 passed\n\n");
}

void test5() {
  printf("Running parser test 5...\n");

  const char *source = "int x;";
  Lexer lexer = new_lexer(source);
  Scope scope;
  scope.parent_scope = nullptr;

  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Int);

  ASTNode *node = parse_declaration(&lexer, &scope);

  assert(node);
  assert(node->type == ASTNodeType::Declaration);
  assert(node->object->name == "x");
  assert(get_current_token(&lexer)->type == TokenType::Eof);

  printf("test 5 passed\n\n");
}

void test6() {
  printf("Running parser test 6...\n");

  const char *source = "int x = 5;";
  Lexer lexer = new_lexer(source);
  Scope scope;
  scope.parent_scope = nullptr;

  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Int);

  ASTNode *node = parse_declaration(&lexer, &scope);

  assert(node);
  assert(node->type == ASTNodeType::Declaration);
  assert(node->object->name == "x");
  assert(get_current_token(&lexer)->type == TokenType::Eof);

  fprintf(stderr, "FIXME: Parse initializers, data structure for initializers");
  // printf("test 6 passed\n\n");
}

int main() {
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
}
