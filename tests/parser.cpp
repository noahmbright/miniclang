#include "parser.h"
#include "lexer.h"
#include "type.h"
#include <cassert>

void test1()
{
  printf("Running parser test 1...\n");

  char const* source = "1";
  Lexer lexer = new_lexer(source);

  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Number);

  Scope scope;
  scope.parent_scope = nullptr;
  ASTNode* node = parse_expression(&lexer, &scope);
  assert(node->data_as.int_data == 1);
  assert(node->lhs == nullptr);
  assert(node->rhs == nullptr);
  assert(get_current_token(&lexer)->type == TokenType::Eof);
  assert(*lexer.current_location == '\0');

  printf("test 1 passed\n\n");
}

void test2()
{
  printf("Running parser test 2...\n");

  char const* source = "20";
  Lexer lexer = new_lexer(source);
  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Number);

  Scope scope;
  scope.parent_scope = nullptr;
  ASTNode* node = parse_expression(&lexer, &scope);
  assert(node->data_as.int_data == 20);
  assert(node->lhs == nullptr);
  assert(node->rhs == nullptr);
  assert(get_current_token(&lexer)->type == TokenType::Eof);
  assert(*lexer.current_location == '\0');

  printf("test 2 passed\n\n");
}

void test3()
{
  printf("Running parser test 3...\n");

  char const* source = "20 * 6";
  Lexer lexer = new_lexer(source);
  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Number);

  Scope scope;
  scope.parent_scope = nullptr;
  ASTNode* node = parse_expression(&lexer, &scope);
  assert(node->type == ASTNodeType::Multiplication);
  assert(node->lhs->data_as.int_data == 20);
  assert(node->rhs->data_as.int_data == 6);
  assert(get_current_token(&lexer)->type == TokenType::Eof);
  assert(*lexer.current_location == '\0');

  printf("test 3 passed\n\n");
}

void test4()
{
  printf("Running parser test 4...\n");

  char const* source = "20 * 6123 / 330 % 2";
  Lexer lexer = new_lexer(source);
  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Number);

  Scope scope;
  scope.parent_scope = nullptr;
  ASTNode* node = parse_expression(&lexer, &scope);

  assert(node->type == ASTNodeType::Modulo);
  assert(node->rhs->data_as.int_data == 2);

  ASTNode* div_node = node->lhs;
  assert(div_node->type == ASTNodeType::Division);
  assert(div_node->rhs->data_as.int_data == 330);

  ASTNode* mod_node = div_node->lhs;
  assert(mod_node->type == ASTNodeType::Multiplication);
  assert(mod_node->lhs->data_as.int_data == 20);
  assert(mod_node->rhs->data_as.int_data == 6123);

  assert(get_current_token(&lexer)->type == TokenType::Eof);
  assert(*lexer.current_location == '\0');

  printf("test 4 passed\n\n");
}

void test5()
{
  printf("Running parser test 5...\n");

  char const* source = "int x;";
  Lexer lexer = new_lexer(source);
  Scope scope;
  scope.parent_scope = nullptr;

  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Int);

  ASTNode* node = parse_declaration(&lexer, &scope);

  assert(node);
  assert(node->type == ASTNodeType::Declaration);
  assert(node->object->identifier == "x");
  assert(get_current_token(&lexer)->type == TokenType::Eof);

  printf("test 5 passed\n\n");
}

void test6()
{
  printf("Running parser test 6...\n");

  char const* source = "int x = 5;";
  Lexer lexer = new_lexer(source);
  Scope scope;
  scope.parent_scope = nullptr;

  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Int);

  ASTNode* node = parse_declaration(&lexer, &scope);

  assert(node);
  assert(node->type == ASTNodeType::Declaration);
  assert(node->object->identifier == "x");
  assert(get_current_token(&lexer)->type == TokenType::Eof);

  fprintf(stderr, "FIXME: Parse initializers, data structure for initializers\n\n");
  // printf("test 6 passed\n\n");
}

void test7()
{
  printf("Running parser test 7...\n");

  char const* source = "int *x;";
  Lexer lexer = new_lexer(source);
  Scope scope;
  scope.parent_scope = nullptr;

  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Int);

  ASTNode* node = parse_declaration(&lexer, &scope);

  // type should be pointer to int
  assert(node);
  assert(node->type == ASTNodeType::Declaration);
  assert(node->object->identifier == "x");
  assert(node->object->type->fundamental_type == FundamentalType::Pointer);
  assert(node->object->type->pointed_type == get_fundamental_type_pointer(FundamentalType::Int));

  assert(get_current_token(&lexer)->type == TokenType::Eof);

  printf("test 7 passed\n\n");
}

void test8()
{
  printf("Running parser test 8...\n");

  char const* source = "int x();";
  Lexer lexer = new_lexer(source);
  Scope scope;
  scope.parent_scope = nullptr;

  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Int);

  ASTNode* node = parse_declaration(&lexer, &scope);

  assert(node);
  assert(node->type == ASTNodeType::Declaration);
  assert(node->object->identifier == "x");

  assert(node->object->type->function_data->return_type == get_fundamental_type_pointer(FundamentalType::Int));
  assert(node->object->type->function_data->parameter_list == nullptr);
  assert(node->object->type->fundamental_type == FundamentalType::Function);

  assert(get_current_token(&lexer)->type == TokenType::Eof);

  printf("test 8 passed\n\n");
}

void test9()
{
  printf("Running parser test 9: Compound statement...\n");

  char const* source = "{int x;\nchar* s;}";
  Lexer lexer = new_lexer(source);
  Scope scope;
  scope.parent_scope = nullptr;

  get_next_token(&lexer);

  ASTNode* node = parse_statement(&lexer, &scope);

  assert(node);
  assert(node->type == ASTNodeType::Declaration);
  assert(node->object->identifier == "x");

  assert(node->object->type == get_fundamental_type_pointer(FundamentalType::Int));

  ASTNode* next_node = node->next;
  assert(next_node);
  assert(next_node->object->identifier == "s");
  assert(next_node->object->type->pointed_type == get_fundamental_type_pointer(FundamentalType::Char));
  assert(next_node->object->type->fundamental_type == FundamentalType::Pointer);

  assert(get_current_token(&lexer)->type == TokenType::Eof);

  printf("test 9 passed\n\n");
}

void test10()
{
  printf("Running parser test 10...\n");

  char const* source = "float x = 5.0;";
  Lexer lexer = new_lexer(source);
  Scope scope;
  scope.parent_scope = nullptr;

  get_next_token(&lexer);
  assert(get_current_token(&lexer)->type == TokenType::Float);

  ASTNode* node = parse_declaration(&lexer, &scope);

  assert(node);
  assert(node->type == ASTNodeType::Declaration);
  assert(node->object->identifier == "x");
  assert(get_current_token(&lexer)->type == TokenType::Eof);

  fprintf(stderr, "FIXME: Parse initializers, data structure for initializers\n\n");
  // printf("test 10 passed\n\n");
}

void test11()
{
  printf("Running parser test 11: Translation unit...\n");

  char const* source = "void function(int x){ double y = 4;\nreturn y; }\n float z = 3; ";
  ExternalDeclaration* declaration = parse_translation_unit(source);

  assert(declaration);
  assert(declaration->type == ExternalDeclarationType::FunctionDefinition);
  assert(declaration->root_ast_node);
  assert(declaration->next);

  ASTNode const* function_ast_node = declaration->root_ast_node;
  assert(function_ast_node->object->type->function_data->return_type == get_fundamental_type_pointer(FundamentalType::Void));
  assert(function_ast_node->object->type->function_data->parameter_list->parameter_type == get_fundamental_type_pointer(FundamentalType::Int));

  ASTNode const* function_body = function_ast_node->object->function_body;
  assert(function_body);
  assert(function_body->type == ASTNodeType::Declaration);
  assert(function_body->object->identifier == "y");
  assert(function_body->object->type == get_fundamental_type_pointer(FundamentalType::Double));

  ExternalDeclaration* next_node = declaration->next;
  assert(next_node->type == ExternalDeclarationType::Declaration);
  assert(next_node->root_ast_node);

  ASTNode const* float_node = next_node->root_ast_node;
  assert(float_node->object->identifier == "z");
  assert(float_node->object->type->fundamental_type == FundamentalType::Float);

  printf("test 11 passed\n\n");
}

int main()
{
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();
  test8();
  test9();
  // test10();
  test11();
}
