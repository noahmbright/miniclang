#include "parser.h"
#include "lexer.h"

ASTNode *new_ast_node() {
  ASTNode *new_node = (ASTNode *)malloc(sizeof(ASTNode));
  return new_node;
}

static bool variable_in_scope(String variable_name, Scope *scope) {
  for (Scope *current_scope = scope; current_scope != nullptr;
       current_scope = current_scope->parent_scope)
    if (current_scope->variables.contains(variable_name))
      return true;

  return false;
}

static bool type_in_scope(String type_name, Scope *scope) {
  for (Scope *current_scope = scope; current_scope != nullptr;
       current_scope = current_scope->parent_scope)
    if (current_scope->typedef_names.contains(type_name))
      return true;

  return false;
}

// 6.5.16
// Assignment expression: conditional expression
//                        unary-expression assignment-operator
//                        assignment-expression

// Parsing declarations
// declaration: declaration-specifier optional-init-declarator-list ;

// the next token_is_* functions check for the different type of
// declaration specifiers
static bool token_is_type_qualifier(Token token) {
  using enum TokenType;
  switch (token.type) {
  case Const:
  case Restrict:
  case Volatile:
  case Atomic:
    return true;
  default:
    return false;
  }
}

static bool token_is_storage_class_specifier(Token token) {
  using enum TokenType;
  switch (token.type) {
  case Typedef:
  case Extern:
  case Static:
  case ThreadLocal:
  case Auto:
  case Register:
    return true;
  default:
    return false;
  }
}

static bool token_is_alignment_specifier(Token token) {
  return token.type == TokenType::AlignAs;
}

static bool token_is_function_specifier(Token token) {
  using enum TokenType;
  return token.type == Inline || token.type == TokenType::NoReturn;
}

static bool token_is_type_specifier(Token token, Scope *scope) {
  using enum TokenType;
  switch (token.type) {
  case Void:
  case Char:
  case Short:
  case Int:
  case Long:
  case Float:
  case Double:
  case Signed:
  case Unsigned:
  case Bool:
  case Complex:
  case Atomic:
  case Struct:
  case Enum:
  case Union:
    return true;
  default:
    return type_in_scope(token.string, scope);
  }
}

static bool token_is_declaration_specifier(Token token, Scope *scope) {
  return token_is_storage_class_specifier(token) ||
         token_is_type_specifier(token, scope) ||
         token_is_type_qualifier(token) || token_is_function_specifier(token) ||
         token_is_alignment_specifier(token);
}

void parse_declaration_specifiers(Lexer *lexer) {
  // FIXME: where to store type info?
}

// 6.7.6 Declarators
// Declarations end with an init-declarator-list
//
// init-declarator-list: init-declarator
//                       init-declarator-list init-declarator
//
// init-declarator: declarator
//                  declarator = initializer
//
// declarator: pointer(optional) direct-declarator
//
// pointer: * type-qualifier-list(optional)
//          * type-qualifier-list(optional) pointer
//

// TODO: Wrangle types and figure out what to do with
// products of this function
void parse_pointer(Lexer *lexer) {
  Token token_after_asterisk = expect_and_skip(lexer, TokenType::Asterisk,
                                               "Parsing pointer, expected *");

  if (token_after_asterisk.type == TokenType::Asterisk)
    parse_pointer(lexer);

  else if (token_is_type_qualifier(token_after_asterisk))
    // FIXME:;
    ;
}

// direct-declarator:

void parse_declarator() {}

// TODO 6.7.7 Type names

void parse_declaration() {}

void parse_file(const char *file) {
  Lexer lexer = new_lexer(file);
  Scope current_scope;

  for (Token current_token = next_token(&lexer);
       current_token.type != TokenType::Eof;
       current_token = next_token(&lexer)) {

    if (token_is_declaration_specifier(current_token, &current_scope)) {
      parse_declaration();
    }
  }
}
