#pragma once

#include "lexer.h"
#include "type.h"

#include <cstdlib>
#include <string>
#include <unordered_set>

struct ASTNode;

struct Scope {
  Scope *parent_scope;

  std::unordered_set<std::string> variables;
  std::unordered_set<std::string> typedef_names;
};

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
  const Type *type;
  ASTNode *function_body;
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

  ASTNode *next;
  ASTNode *lhs;
  ASTNode *rhs;

  // for ternary conditional
  ASTNode *conditional;

  // declarations/definitions
  Object *object;
};

ASTNode *new_ast_node(ASTNodeType);
bool expect_token_type(Token *, TokenType);

const Type *declaration_to_fundamental_type(DeclarationSpecifierFlags *);

// expressions
ASTNode *parse_expression(Lexer *);
ASTNode *parse_primary_expression(Lexer * /*, Scope *scope*/);
ASTNode *parse_assignment_expression(Lexer * /*, Scope *scope*/);

// declarations
bool token_is_declaration_specifier(const Token *, Scope *);
ASTNode *parse_declaration(Lexer *, Scope *);
Object *parse_declarator(Lexer *, const Type *, Scope *);
ASTNode *parse_init_declarator(Lexer *);
const Type *parse_pointer(Lexer *, const Type *);

ASTNode *parse_initializer(Lexer *);
ASTNode *parse_initializer_list(Lexer *);
DeclarationSpecifierFlags parse_declaration_specifiers(Lexer *, Scope *);

void parse_rest_of_declaration(Lexer *, Scope *, ASTNode *);

// statements
ASTNode *parse_statement(Lexer *lexer, Scope *scope);

ASTNode *parse_translation_unit(const char *);
