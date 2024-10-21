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
};

ASTNode *new_ast_node(ASTNodeType);
bool expect_token_type(Token *token, TokenType type);
Token expect_and_skip(Lexer *lexer, Token token, TokenType type, const char *);

void parse_pointer(Lexer *);
