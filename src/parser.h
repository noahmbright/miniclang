#pragma once

#include "lexer.h"
#include "mini_string.h"

#include <cstdlib>
#include <unordered_set>

struct Scope {
  Scope *parent_scope;

  std::unordered_set<String> variables;
  std::unordered_set<String> typedef_names;
};

enum class DataType {
  Char,
  Short,
  Int,
  Long,
  Float,
  Doublem,
  Bool,
  Enum,
  Struct,
  Void,
  Function,
  Union,
  Pointer
};

struct Object {};

struct ASTNode {
  ASTNode *lhs;
  ASTNode *rhs;

  Object *object;
};

ASTNode *new_ast_node();
bool expect_token_type(Token token, TokenType type);
Token expect_and_skip(Lexer *lexer, Token token, TokenType type, const char *);
