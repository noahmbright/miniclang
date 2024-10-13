#include "parser.h"
#include "lexer.h"
#include "type.h"

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
static bool token_is_type_qualifier(Token *token) {
  using enum TokenType;
  switch (token->type) {
  case Const:
  case Restrict:
  case Volatile:
  case Atomic:
    return true;
  default:
    return false;
  }
}

// At most, one storage-class specifier may be given in the declaration
// specifiers in a declaration, except that _Thread_local may appear with static
// or extern.
static bool token_is_storage_class_specifier(Token *token) {
  using enum TokenType;
  switch (token->type) {
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

static bool token_is_alignment_specifier(Token *token) {
  return token->type == TokenType::AlignAs;
}

static bool token_is_function_specifier(Token *token) {
  using enum TokenType;
  return token->type == Inline || token->type == TokenType::NoReturn;
}

static bool token_is_type_specifier(Token *token, Scope *scope) {
  using enum TokenType;
  switch (token->type) {
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
    return type_in_scope(token->string, scope);
  }
}

static bool token_is_declaration_specifier(Token *token, Scope *scope) {
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
void parse_declarator() {}

// pointer: * type-qualifier-list(optional)
//          * type-qualifier-list(optional) pointer
static void parse_type_qualifier_list(Lexer *lexer) {}

// TODO: Wrangle types and figure out what to do with
// products of this function
void parse_pointer(Lexer *lexer) {

  Token *token_after_asterisk = expect_and_skip(lexer, TokenType::Asterisk,
                                                "Parsing pointer, expected *");

  // parse pointer to pointer
  if (token_after_asterisk->type == TokenType::Asterisk)
    parse_pointer(lexer);

  // parse type qualifier list
  else if (token_is_type_qualifier(token_after_asterisk))
    parse_type_qualifier_list(lexer);
}

// direct-declarator:

// 6.7.7 Type names
// type-name:
//   specifier-qualifier-list abstract-declarator(optional)
void parse_typename() {}

// specifier-qualifier-list:
//      specifier-qualifier-list(optional) type-specifiers/qualifier
//
//  if a name appears several times, its the same as if it appears only once
//  if the _Atomic specifier appears, its the same as if it appears at the
//  beginning
//
//  so if a specifier/qualifier appears at all, just set the flag to true
//
//
//  At least one type specifier shall be given in the declaration specifiers in
//  each declaration,  and in the specifier-qualifier list in each struct
//  declaration and type name.
//
static bool token_is_specifier_or_qualifier(Token *token, Scope *scope) {
  return token_is_type_specifier(token, scope) ||
         token_is_type_qualifier(token);
}

SpecifiedType parse_specifier_qualifier_list(Lexer *lexer, Scope *scope) {
  Token *current_token = get_next_token(lexer);
  SpecifiedType specified_type;
  while (token_is_specifier_or_qualifier(current_token, scope)) {
    updated_specified_type(current_token, specified_type);
  }
}

// abstract-declarator:
//      pointer
//      pointer(optional) direct-abstract-declarator

void parse_declaration() {}

void parse_file(const char *file) {
  Lexer lexer = new_lexer(file);
  Scope current_scope;

  for (Token *current_token = get_next_token(&lexer);
       current_token->type != TokenType::Eof;
       current_token = get_next_token(&lexer)) {

    if (token_is_declaration_specifier(current_token, &current_scope)) {
      parse_declaration();
    }
  }
}
