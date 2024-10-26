#include "lexer.h"
#include "parser.h"
#include "type.h"

#include <cassert>

ASTNode *new_ast_node(ASTNodeType type = ASTNodeType::Void) {
  ASTNode *new_node = (ASTNode *)malloc(sizeof(ASTNode));

  new_node->type = type;
  new_node->data_type = DataType::Void;

  new_node->conditional = nullptr;
  new_node->lhs = nullptr;
  new_node->rhs = nullptr;

  return new_node;
}

static Object *new_object() {
  Object *new_object = (Object *)malloc(sizeof(Object));
  new_object->as.variable = {};
  return new_object;
}

/*static bool variable_in_scope(std::string variable_name, Scope *scope) {

  for (Scope *current_scope = scope; current_scope != nullptr;
       current_scope = current_scope->parent_scope)

    if (current_scope->variables.contains(variable_name))
      return true;

  return false;
}*/

static bool typedef_name_in_scope(const std::string &type_name, Scope *scope) {

  for (Scope *current_scope = scope; current_scope != nullptr;
       current_scope = current_scope->parent_scope) {

    if (current_scope->typedef_names.contains(type_name)) {
      return true;
    }
  }

  return false;
}

static bool token_is_type_qualifier(const Token *token) {
  switch (token->type) {
  case TokenType::Const:
  case TokenType::Restrict:
  case TokenType::Volatile:
  case TokenType::Atomic:
    return true;
  default:
    return false;
  }
}

static bool token_is_storage_class_specifier(const Token *token) {
  switch (token->type) {
  case TokenType::Typedef:
  case TokenType::Extern:
  case TokenType::Static:
  case TokenType::ThreadLocal:
  case TokenType::Auto:
  case TokenType::Register:
    return true;
  default:
    return false;
  }
}

static bool token_is_alignment_specifier(const Token *token) {
  return token->type == TokenType::AlignAs;
}

static bool token_is_function_specifier(const Token *token) {
  return token->type == TokenType::Inline || token->type == TokenType::NoReturn;
}

static bool token_is_type_specifier(const Token *token, Scope *scope) {
  switch (token->type) {
  case TokenType::Void:
  case TokenType::Char:
  case TokenType::Short:
  case TokenType::Int:
  case TokenType::Long:
  case TokenType::Float:
  case TokenType::Double:
  case TokenType::Signed:
  case TokenType::Unsigned:
  case TokenType::Bool:
  case TokenType::Complex:
  case TokenType::Atomic:
  case TokenType::Struct:
  case TokenType::Enum:
  case TokenType::Union:
    return true;
  default:
    return typedef_name_in_scope(token->string, scope);
  }
}

static bool token_is_declaration_specifier(const Token *token, Scope *scope) {
  return token_is_storage_class_specifier(token) ||
         token_is_type_specifier(token, scope) ||
         token_is_type_qualifier(token) || token_is_function_specifier(token) ||
         token_is_alignment_specifier(token);
}

// 6.7 Declarations
//
// a declaration is a list of declaration specifiers followed by an init
// declarator list, e.g.
//      const int *x[] = {};
// the declaration specifiers are const int, and the init
// declarator list is the single declarator *x[] = {}
//
// the init declarator list is optional, so e.g. "int;" is a valid declaration,
// but it is just dead code
//
// parsing declarations is mostly done in 6.7.6 declarators
//
// declaration specifiers are type qualifiers, storage class specifiers,
// type specifiers, function specifiers, and alignment specifiers
//
// one set of declaration specifiers applies to each item in the init declarator
// list, so we can cache all those in this DeclarationSpecifierFlags object
DeclarationSpecifierFlags parse_declaration_specifiers(Lexer *lexer,
                                                       Scope *scope) {

  DeclarationSpecifierFlags declaration;
  declaration.flags = 0;

  while (token_is_declaration_specifier(get_current_token(lexer), scope)) {
    update_declaration_specifiers(get_current_token(lexer), &declaration);
    get_next_token(lexer);
  }

  return declaration;
}

// declaration: declaration-specifiers init-declarator-list(opt)
//
// after parsing a declaration, we make an AST node that will
// initialize the new objects that have been declarated
//
// an init-declarator-list is a list of init-declarators
// e.g. int (x, y=5;), the init-declarator-list in parentheses
//
// declarators are separated by commas, so after the first one,
// we need to expect and skip
//
// pass parse_declarator the declaration flags so it can have
// complete type information
ASTNode *parse_declaration(Lexer *lexer, Scope *scope) {

  assert(token_is_declaration_specifier(get_current_token(lexer), scope) &&
         "parse_declaration: starting but current token is not a declaration "
         "specifier");

  ASTNode *ast_node = new_ast_node(ASTNodeType::Declaration);
  DeclarationSpecifierFlags declaration =
      parse_declaration_specifiers(lexer, scope);
  bool already_parsed_first_declarator = false;

  while (get_current_token(lexer)->type != TokenType::Semicolon) {

    if (already_parsed_first_declarator)
      expect_and_get_next_token(
          lexer, TokenType::Comma,
          "Parsing declaration, expected comma or semicolon");
    already_parsed_first_declarator = true;

    ast_node->object = parse_declarator(lexer, &declaration);
  }

  // skip semicolon
  get_next_token(lexer);
  return ast_node;
}

ASTNode *parse_init_declarator(Lexer *lexer) {
  // FIXME
  ASTNode *ast_node = new_ast_node(ASTNodeType::Declaration);

  // Object *object = parse_declarator(lexer);

  if (get_current_token(lexer)->type == TokenType::Equals) {
    parse_initializer(lexer);
  }

  return ast_node;
}

// 6.7.2 Structs, unions, enums

// 6.7.6 Declarators
// Declarations end with an init-declarator-list
// init-declarator: declarator
//                  declarator = initializer
//
//      a declaration may be e.g. int x = 3, y[], z(), *ptr;
//      the declarators are x = 3, y[], z(), and *ptr
//
//      the result of parsing a declarator is a new identifier
//      of a certain type: variable, function, array/ptr
//
// init-declarator-list: init-declarator
//                       init-declarator-list init-declarator
// declarator: pointer(optional) direct-declarator
Object *parse_declarator(Lexer *lexer, DeclarationSpecifierFlags *declaration) {
  DataType type = type_kind_from_declaration(declaration);
  (void)type;

  if (get_current_token(lexer)->type == TokenType::Asterisk) {
    get_next_token(lexer);
    parse_pointer(lexer);
  }

  // FIXME ??

  return parse_direct_declarator(lexer);
}

// e.g. parse a const*
static void parse_type_qualifier_list(Lexer *lexer) {
  const Token *current_token = get_current_token(lexer);

  DeclarationSpecifierFlags declaration;
  // potential FIXME: pass around Declarations from the right places
  while (token_is_type_qualifier(current_token)) {
    update_declaration_specifiers(current_token, &declaration);
    current_token = get_next_token(lexer);
  }
}

// pointer is *type-qualifier-list(opt) followed by another ptr(opt)
// e.g. parse a (int) *const *volatile x;
// x is a volatile pointer to a const pointer to int
//
// pointer: * type-qualifier-list(optional)
//          * type-qualifier-list(optional) pointer
void parse_pointer(Lexer *lexer) {

  const Token *token_after_asterisk = expect_and_get_next_token(
      lexer, TokenType::Asterisk, "Parsing pointer, expected *");

  // parse pointer to pointer
  if (token_after_asterisk->type == TokenType::Asterisk)
    parse_pointer(lexer);

  // parse type qualifier list
  else if (token_is_type_qualifier(token_after_asterisk))
    parse_type_qualifier_list(lexer);
}

// parameter-type-list:
//      parameter-list
//      parameter-list, ...
//
// this is an extra that fixes variadic function parameters
// to come at the end in function signature declarations
// so f(int x, ...) is allowed by (int x, ..., int y) isn't
void parse_parameter_type_list(Lexer *lexer, FunctionType *function) {

  (void)function;
  assert(get_current_token(lexer)->type == TokenType::LParen &&
         "Parsing parameter typelist, but not starting on LParen");

  switch (get_next_token(lexer)->type) {
  case TokenType::Ellipsis:
    expect_and_get_next_token(
        lexer, TokenType::RParen,
        "Expected RParen after ellipsis in variadic argument");
  default:
    return;
  }
}

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
//      (declarator)
//      direct-declarator [type-qual-list(opt) assign-expr(opt)]
//          e.g. x[], x[10], x[const 10]
//
//      direct-declarator [static type-qualifier-list(opt) assignment-expr]
//      direct-declarator [type-qualifier-list static assignment-expression]
//      direct-declarator [type-qualifier-list(opt) * ]
//          these three only allowed in function definitions, the qualifiers
//          have to come first in an array of arrays
//              e.g. x[const 10][10];
//              the second 10 is needed for the array to have a complete type
//
//      direct-declarator (parameter-type-list)
//          e.g. int x(int y, ...), for function declarations
//      direct-declarator (identifier-list(opt))
//          e.g. int x(), int x(int y), for function calls

Object *parse_direct_declarator(Lexer *lexer, Object *object,
                                DeclarationSpecifierFlags *declaration) {
  const Token *current_token = get_current_token(lexer);

  // parenthesis, parse another declarator
  if (current_token->type == TokenType::LParen) {
    get_next_token(lexer);
    parse_declarator(lexer, declaration);
    current_token = expect_and_get_next_token(
        lexer, TokenType::RParen, "parse_direct_declarator expected RParen");
  }

  // start of parsing a new declarator with name identifier's-name
  if (current_token->type == TokenType::Identifier) {
    assert(!object &&
           "parse_direct_declarator: found identifier but object* is not null");

    Object *object = new_object();
    object->name = current_token->string;
    assert(object->name != "" &&
           "parse_direct_declarator: assigned null name string to new object");

    switch (get_next_token(lexer)->type) {
      // hit a semicolon or comma, meaning either end of declaration or next
      // declarator
    case TokenType::Semicolon:
    case TokenType::Comma:
      assert(object && "parse_direct_declarator: returning null object");
      return object;

      // declaring an array
    case TokenType::LBracket:

      // declaring a function
    case TokenType::LParen:
      switch (get_next_token(lexer)->type) {
      case TokenType::Identifier:
      case TokenType::RParen:
        return object;
      default:
      }

    default:
      error_token(lexer, "FIXME: default case in parse_direct_declarator");
    }
  }
  assert(false);
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
static bool token_is_specifier_or_qualifier(const Token *token, Scope *scope) {
  return token_is_type_specifier(token, scope) ||
         token_is_type_qualifier(token);
}

DeclarationSpecifierFlags parse_specifier_qualifier_list(Lexer *lexer,
                                                         Scope *scope) {
  const Token *current_token = get_next_token(lexer);
  DeclarationSpecifierFlags declaration;
  while (token_is_specifier_or_qualifier(current_token, scope)) {
    update_declaration_specifiers(current_token, &declaration);
    current_token = get_next_token(lexer);
  }
  return declaration;
}

// abstract declarators are used when the identifier name is irrelevant
// so in type names and in function declarations
// e.g. int * x[] declares x with type int * []
// and you can declare a function with signature f(int*);

// direct-abstract-declarator:
//      ( abstract-declarator )
//
//      direct-abstract-declarator(opt) [ type-qualifier-list(opt)
//              assignment-expression(opt) ]
//
//      direct-abstract-declarator(opt) [ static type-qualifier-list(opt)
//           assignment-expression ]
//
//      direct-abstract-declarator(opt) [ type-qualifier-list static
//           assignment-expression ]
//
//      direct-abstract-declarator(opt) [*]
//
//      direct-abstract-declarator(opt) ( parameter-type-list(opt) )
void parse_direct_abstract_declarator(Lexer *lexer) {
  // parenthesis
  if (get_current_token(lexer)->type == TokenType::LParen) {
    get_next_token(lexer);
    parse_abstract_declarator(lexer);
    expect_token_type(get_current_token(lexer), TokenType::RParen);
  }
}

// abstract-declarator:
//      pointer
//      pointer(optional) direct-abstract-declarator
AbstractType *parse_abstract_declarator(Lexer *lexer) {
  AbstractType *abstract_type = new_abstract_type();
  if (get_current_token(lexer)->type == TokenType::Asterisk)
    parse_pointer(lexer);

  return abstract_type;
}

// 6.7.9 Initialization
void parse_initializer_list(Lexer *lexer) { (void)lexer; }

// initializer:
//      assignment-expression
//      { initializer-list }
//      { initializer-list, }
ASTNode *parse_initializer(Lexer *lexer) {
  if (get_current_token(lexer)->type == TokenType::LBracket)
    // FIXME:
    parse_initializer_list(lexer);

  return parse_assignment_expression(lexer);
}

void parse_file(const char *file) {
  Lexer lexer = new_lexer(file);
  Scope current_scope;

  for (const Token *current_token = get_next_token(&lexer);
       current_token->type != TokenType::Eof;
       current_token = get_next_token(&lexer)) {

    if (token_is_declaration_specifier(current_token, &current_scope)) {
      parse_declaration(&lexer, &current_scope);
    }
  }
}

// 6.8 Statements
//      labeled statement
//      compound statement
//      expression statement
//      selection statement
//      iteration statement
//      jump statement

// labeled statements are
//      identifier : statement for use with goto
//  or case const-expression : statement/ default :statement for switches

// compound statement are blocks of declarations and other statements wrapped in
// {}, for use in basically everything, e.g. for loops

// expression statements are expr(opt);

// selection statements are ifs/switches

// iteration statements are (do) while and for

// jumps are goto identifier; continue; break; return;
