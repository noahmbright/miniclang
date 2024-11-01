#pragma once

#include "lexer.h"
#include "type.h"

#include <cstdlib>
#include <string>
#include <unordered_map>

struct ASTNode;

enum class ASTNodeType {
  Void,

  // primary expressions
  NumericConstant,
  Variable,

  // binary expressions
  Multiplication,
  Division,
  Modulo,
  Addition,
  Subtraction,
  BitShiftLeft,
  BitShiftRight,
  GreaterThan,
  GreaterThanOrEqualTo,
  LessThan,
  LessThanOrEqualTo,
  EqualityComparison,
  InequalityComparison,
  BitwiseAnd,
  BitwiseXor,
  BitwiseOr,
  LogicalAnd,
  LogicalOr,
  ConditionalExpression,
  Assignment,

  // declarations
  Declaration
};

// functions or variables
struct Object {
  std::string identifier;
  Type const* type;
  ASTNode* function_body;
};

struct ASTNode {
  ASTNodeType type;

  FundamentalType data_type;

  union {
    char char_data;

    short int short_data;
    unsigned short unsigned_short_data;
    int int_data;
    unsigned int unsigned_int_data;
    long int long_data;
    unsigned long int unsigned_long_data;
    long long long_long_data;
    unsigned long long unsigned_long_long_data;
    float float_data;
    double double_data;
    long double long_double_data;

  } data_as;

  ASTNode* next;
  ASTNode* lhs;
  ASTNode* rhs;

  // for ternary conditional
  ASTNode* conditional;

  // declarations/definitions
  Object* object;
};

struct Scope {
  Scope* parent_scope;
  std::unordered_map<std::string, Object*> variables;
  std::unordered_map<std::string, Object*> typedef_names;
};

enum class ExternalDeclarationType {
  FunctionDefinition,
  Declaration
};

struct ExternalDeclaration {
  ExternalDeclaration* next;
  ExternalDeclarationType type;
  ASTNode const* root_ast_node;
};

ASTNode* new_ast_node(ASTNodeType);
bool expect_token_type(Token*, TokenType);

Type const* declaration_to_fundamental_type(DeclarationSpecifierFlags*);

Object* variable_in_scope(std::string const&, Scope*);

// expressions

ASTNode* parse_expression(Lexer*, Scope*);
ASTNode* parse_primary_expression(Lexer*, Scope*);
ASTNode* parse_assignment_expression(Lexer*, Scope*);

// declarations
bool token_is_declaration_specifier(Token const*, Scope*);
ASTNode* parse_declaration(Lexer*, Scope*);
Object* parse_declarator(Lexer*, Type const*, Scope*);
ASTNode* parse_init_declarator(Lexer*);
Type const* parse_pointer(Lexer*, Type const*);

ASTNode* parse_initializer(Lexer*, Scope*);
ASTNode* parse_initializer_list(Lexer*, Scope*);
DeclarationSpecifierFlags parse_declaration_specifiers(Lexer*, Scope*);

void parse_rest_of_declaration(Lexer*, Scope*, ASTNode*);

// statements
ASTNode* parse_statement(Lexer* lexer, Scope* scope);

ExternalDeclaration* parse_translation_unit(char const*);
