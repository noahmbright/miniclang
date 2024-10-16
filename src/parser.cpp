#include "parser.h"
#include "lexer.h"
#include "type.h"

#include <cassert>

ASTNode *new_ast_node() {
  ASTNode *new_node = (ASTNode *)malloc(sizeof(ASTNode));
  return new_node;
}

static bool variable_in_scope(std::string variable_name, Scope *scope) {
  for (Scope *current_scope = scope; current_scope != nullptr;
       current_scope = current_scope->parent_scope)
    if (current_scope->variables.contains(variable_name))
      return true;

  return false;
}

static bool type_in_scope(std::string type_name, Scope *scope) {
  for (Scope *current_scope = scope; current_scope != nullptr;
       current_scope = current_scope->parent_scope)
    if (current_scope->typedef_names.contains(type_name))
      return true;

  return false;
}

// 6.5.1
// primary expressions
//      identifier - lvalues or function designator
//      constant
//      string-literal
//      (expression)
//      generic-selection

// 6.5.16
// Assignment expression: conditional expression
//                        unary-expression assignment-operator
//                        assignment-expression

// 6.7
// Parsing declarations
// declaration: declaration-specifier init-declarator-list(optional) ;

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

Declaration parse_declaration_specifiers(Lexer *lexer, Scope *scope) {
  Token *current_token = get_current_token(lexer);
  Declaration declaration;

  while (token_is_declaration_specifier(current_token, scope)) {
    update_declaration(current_token, &declaration);
    current_token = get_next_token(lexer);
  }

  return declaration;
}

// declaration: declaration-specs init-declarator-list(opt)
// conceptually, a declaration says that in the current scope,
// a new identifier of a certain type is now available
void parse_declaration(Lexer *lexer, Scope *scope) {
  Token *current_token = get_current_token(lexer);
  assert(token_is_declaration_specifier(current_token, scope));

  Declaration declaration = parse_declaration_specifiers(lexer, scope);
}

// 6.7.6 Declarators
// Declarations end with an init-declarator-list
// init-declarator: declarator
//                  declarator = initializer
//
//      this sounds cyclic but the "declarator" is the specification
//      of a new identifier of either variable, pointer, array or
//      function "flavor"
//          I say flavor instead of type because type means something
//          particular
//      e.g. x, x[], x()
//
// init-declarator-list: init-declarator
//                       init-declarator-list init-declarator

// declarator: pointer(optional) direct-declarator
void parse_declarator(Lexer *lexer) {
  Token *current_token = get_current_token(lexer);
  if (current_token->type == TokenType::Asterisk)
    parse_pointer(lexer);

  // parse_direct_declarator();
}

// pointer: * type-qualifier-list(optional)
//          * type-qualifier-list(optional) pointer

// intermediate stop: type qualifier list
// e.g. parse a const*
static void parse_type_qualifier_list(Lexer *lexer) {
  Token *current_token = get_current_token(lexer);

  Declaration declaration;
  // potential FIXME: pass around Declarations from the right places
  while (token_is_type_qualifier(current_token)) {
    update_declaration(current_token, &declaration);
    current_token = get_next_token(lexer);
  }
}

// TODO: Wrangle types and figure out what to do with
// products of this function
// pointer is *type-qualifier-list(opt) followed by another ptr(opt)
// e.g. parse a (int) *const *volatile x;
// x is a volatile pointer to a const int
void parse_pointer(Lexer *lexer) {

  Token *token_after_asterisk = expect_and_get_next_token(
      lexer, TokenType::Asterisk, "Parsing pointer, expected *");

  // parse pointer to pointer
  if (token_after_asterisk->type == TokenType::Asterisk)
    parse_pointer(lexer);

  // parse type qualifier list
  else if (token_is_type_qualifier(token_after_asterisk))
    parse_type_qualifier_list(lexer);
}

// abstract-declarator
//      pointer
//      pointer(opt) direct-abstract-declarator

// direct-abstract-declarator (DAD)
//      (abstract-dec)
//      DAD(opt) [type-qual-list(opt) assign-expr(opt)]

// parameter-type-list
//      parameter-list
//      parameter-list, ...
// this is an extra that fixes variadic function parameters
// to come at the end
// so f(int x, ...) is allowed by (int x, ..., int y) isn't

// parameter-list
//      parameter-declaration
//      parameter-list, parameter-declaration

// parameter-declaration:
//      declaration-specifiers declarator
//      declaration-specifiers abstract-declarator(opt)

// direct declarators can most simply appear in this context:
//      type-specifier direct-declarator
// a direct declarator is one identifier, potentially wrapped in parens,
// followed by array or function qualifications
//
// direct-declarator:
//      identifier
//          e.g. x;
//
//      (declarator)
//          e.g. (x)
//
//      direct-declarator [type-qual-list(opt) assign-expr(opt)]
//          e.g. x[], x[const 10]
//
//      direct-declarator [static type-qualifier-list(opt) assignment-expr]
//          e.g. x[static const 10]
//
//      direct-declarator [type-qualifier-list static assignment-expression]
//          e.g. x[const static 10]
//
//      direct-declarator [type-qualifier-list(opt) * ]
//          e.g. x[const *]
//            used in function declarations for arrays of unknown size
//
//      direct-declarator (parameter-type-list)
//          int x(int y, ...)
//
//      direct-declarator ( identifier-list(opt))
//          int x(), int x(int y)
void parse_direct_declarator(Lexer *lexer) {
  Token *current_token = get_current_token(lexer);

  if (current_token->type == TokenType::LParen) {
    parse_declarator(lexer);
    current_token = expect_and_get_next_token(
        lexer, TokenType::RParen, "parse_direct_declarator expected RParen");
  }

  if (current_token->type == TokenType::Identifier) {

    switch (get_next_token(lexer)->type) {
    case TokenType::Semicolon:
      return;
    // case TokenType::LBracket:
    default:
      error_token(lexer, "FIXME: default case in parse_direct_declarator");
    }
  }
}

// 6.7.7 Type names
// type-name:
//   specifier-qualifier-list abstract-declarator(optional)
//      spec-qual-list is like const int
//      the simplest abstract dec is * or (*)
//      a more complicated one is [*]

void parse_typename() {}

// specifier-qualifier-list:
//      specifier-qualifier-list(optional) type-specifiers/qualifier
static bool token_is_specifier_or_qualifier(Token *token, Scope *scope) {
  return token_is_type_specifier(token, scope) ||
         token_is_type_qualifier(token);
}

Declaration parse_specifier_qualifier_list(Lexer *lexer, Scope *scope) {
  Token *current_token = get_next_token(lexer);
  Declaration declaration;
  while (token_is_specifier_or_qualifier(current_token, scope)) {
    update_declaration(current_token, &declaration);
    current_token = get_next_token(lexer);
  }
  return declaration;
}

// abstract declarators are used when the identifier name is irrelevant
// so in type names and in function declarations
// e.g. int * x[] declares x with type int * []
// and you can declare a function with signature f(int*);

// abstract-declarator:
//      pointer
//      pointer(optional) direct-abstract-declarator

// direct-abstract-declarator:
//      ( abstract-declarator )
//      direct-abstract-declaratoropt [ type-qualifier-list(opt)
//              assignment-expression(opt) ]
//      direct-abstract-declaratoropt [ static type-qualifier-list(opt)
//           assignment-expression ]
//      direct-abstract-declaratoropt [ type-qualifier-list static
//           assignment-expression ]
//      direct-abstract-declarator(opt) [*]
//      direct-abstract-declarator(opt) ( parameter-type-list(opt) )

void parse_file(const char *file) {
  Lexer lexer = new_lexer(file);
  Scope current_scope;

  for (Token *current_token = get_next_token(&lexer);
       current_token->type != TokenType::Eof;
       current_token = get_next_token(&lexer)) {

    if (token_is_declaration_specifier(current_token, &current_scope)) {
      parse_declaration(&lexer, &current_scope);
    }
  }
}
