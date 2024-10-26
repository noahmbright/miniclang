#pragma once

#include "lexer.h"
#include "type.h"

#include <cstdlib>
#include <string>
#include <unordered_set>

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

struct ASTNode {
  ASTNodeType type;

  DataType data_type;

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
Token expect_and_skip(Lexer *, Token, TokenType, const char *);

// expressions
ASTNode *parse_expression(Lexer *);
ASTNode *parse_primary_expression(Lexer * /*, Scope *scope*/);
ASTNode *parse_assignment_expression(Lexer * /*, Scope *scope*/);

// declarations
ASTNode *parse_declaration(Lexer *, Scope *);
void parse_direct_abstract_declarator(Lexer *);
ASTNode *parse_initializer(Lexer *);
Object *parse_declarator(Lexer *, DeclarationSpecifierFlags *);
ASTNode *parse_init_declarator(Lexer *);
AbstractType *parse_abstract_declarator(Lexer *);
Object *parse_direct_declarator(Lexer *, Object * = nullptr,
                                DeclarationSpecifierFlags * = nullptr);
void parse_pointer(Lexer *);
