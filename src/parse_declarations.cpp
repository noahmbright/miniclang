#include "lexer.h"
#include "parser.h"
#include "type.h"

#include <cassert>

static void error_and_stop_parsing(const char *message) {
  fprintf(stderr, "%s", message);
  exit(1);
}

ASTNode *new_ast_node(ASTNodeType type = ASTNodeType::Void) {
  ASTNode *new_node = (ASTNode *)malloc(sizeof(ASTNode));

  new_node->type = type;
  new_node->data_type = FundamentalType::Void;

  new_node->conditional = nullptr;
  new_node->lhs = nullptr;
  new_node->rhs = nullptr;
  new_node->next = nullptr;

  return new_node;
}

static const Object *new_object(const std::string &identifier,
                                const Type *type) {
  Object *new_object = (Object *)malloc(sizeof(Object));
  new_object->identifier = identifier;
  new_object->type = type;

  return new_object;
}

static const FunctionData *
new_function_data(const Type *return_type,
                  const FunctionParameter *parameter_list, bool is_variadic) {
  FunctionData *new_function_type =
      (FunctionData *)malloc(sizeof(FunctionData));

  new_function_type->return_type = return_type;
  new_function_type->parameter_list = parameter_list;
  new_function_type->is_variadic = is_variadic;

  return new_function_type;
}

static FunctionParameter *new_function_parameter(const Type *parameter_type) {
  FunctionParameter *new_parameter =
      (FunctionParameter *)malloc(sizeof(FunctionParameter));

  new_parameter->parameter_type = parameter_type;

  return new_parameter;
};

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
// e.g. int x, y=5;, the init-declarator-list is x, y=5;
//
// declarators are separated by commas, so after the first one,
// we need to expect and skip
//
// a declaration can declare several variables
// handle this by producing a linked list of AST nodes
ASTNode *parse_declaration(Lexer *lexer, Scope *scope) {

  assert(token_is_declaration_specifier(get_current_token(lexer), scope) &&
         "parse_declaration: first token is not a declaration specifier");

  ASTNode ast_node_anchor;
  ast_node_anchor.next = nullptr;
  ASTNode *previous_ast_node = &ast_node_anchor;

  // get the declspecs, e.g. the const int
  DeclarationSpecifierFlags declaration =
      parse_declaration_specifiers(lexer, scope);

  // with the exception of maybe a parenthesized declarator, at this point we
  // know we'll have a (array of/function returning/pointer to) const int
  //
  // we might also have a function like: const int* f(), so we'll ultimately
  // want to return a declaration of a function with return type pointing to a
  // pointer to int
  //
  // FIXME: Where do qualifiers like const go?
  FundamentalType fundamental_type =
      fundamental_type_from_declaration(&declaration);
  const Type *fundamental_type_ptr =
      get_fundamental_type_pointer(fundamental_type);

  // now need to work through the declarators: pointers/arrays and any
  // initializations, which will all have the same declspecs
  bool already_parsed_first_declarator = false;
  while (get_current_token(lexer)->type != TokenType::Semicolon) {

    // check for commas after first declarator
    if (already_parsed_first_declarator)
      expect_and_get_next_token(
          lexer, TokenType::Comma,
          "Parsing declaration, expected comma or semicolon");
    else
      already_parsed_first_declarator = true;

    // make new node with object from declarator
    ASTNode *current_ast_node = new_ast_node(ASTNodeType::Declaration);
    current_ast_node->object =
        parse_declarator(lexer, fundamental_type_ptr, scope);

    // new identifier is explicitly initialized - get initializer
    if (get_current_token(lexer)->type == TokenType::Equals) {
      get_next_token(lexer);
      parse_initializer(lexer);
    }

    previous_ast_node->next = current_ast_node;
    previous_ast_node = previous_ast_node->next;
  } // end while loop

  // skip semicolon
  assert(get_current_token(lexer)->type == TokenType::Semicolon);
  expect_and_get_next_token(lexer, TokenType::Semicolon,
                            "Expected semicolon at end of declaration\n");
  return ast_node_anchor.next;
}

// 6.7.2 Structs, unions, enums

// 6.7.6 Declarators

static const Type *parse_array_dimensions(Lexer *lexer) {
  assert(get_current_token(lexer)->type == TokenType::LBracket);
  // FIXME
  assert(false);
}

// parameter-list: (parameter-declaration)*
//
// the grammar defines an intermediate 'parameter-type-list' production
// but ultimately its purpose is just to stop when you hit ellipsis
//
// parameter-declaration:
//      declaration-specifiers declarator
//      declaration-specifiers abstract-declarator(opt)
//
// abstract declarators allow for parameter lists with variable names omitted
// abstract and concrete declarators both begin with optional pointers
// the presence/absence of an identifier can be used to disambiguate
//
// this function returns a function pointer type
static const Type *parse_parameter_list(Lexer *lexer, const Type *return_type,
                                        Scope *scope) {
  assert(get_current_token(lexer)->type == TokenType::LParen);
  get_next_token(lexer);

  if (scope->parent_scope)
    error_and_stop_parsing(
        "Function declaration only allowed in global scope\n");

  Type *function_type = new_type(FundamentalType::Function);

  // making/traversing linked list of function params
  FunctionParameter parameter_list_anchor;
  parameter_list_anchor.next_parameter = nullptr;
  FunctionParameter *previous_parameter = &parameter_list_anchor;

  bool is_variadic = false;
  bool parsed_first_parameter_yet = false;

  while (get_current_token(lexer)->type != TokenType::RParen) {

    // check commas between parameters
    if (parsed_first_parameter_yet)
      expect_and_get_next_token(
          lexer, TokenType::Comma,
          "Parsing parameter list, expected comma or right parenthesis");
    else
      parsed_first_parameter_yet = true;

    // variadic, check rparen and stop
    if (get_current_token(lexer)->type == TokenType::Ellipsis) {
      is_variadic = true;
      expect_and_get_next_token(lexer, TokenType::RParen,
                                "Parsing parameter list, expected right "
                                "parenthesis after ellipsis\n");
      return function_type;
    }

    // regular parameter, definitely starting with a type specifier
    DeclarationSpecifierFlags flags =
        parse_declaration_specifiers(lexer, scope);

    const Type *parameter_type =
        get_fundamental_type_pointer(fundamental_type_from_declaration(&flags));

    // potentially a pointer argument
    if (get_current_token(lexer)->type == TokenType::Asterisk)
      parameter_type = parse_pointer(lexer, parameter_type);

    // potentially has an identifier, skip
    if (get_current_token(lexer)->type == TokenType::Identifier)
      get_next_token(lexer);

    // FIXME: finally either a function pointer or array parameter

    FunctionParameter *current_function_parameter =
        new_function_parameter(parameter_type);

    previous_parameter->next_parameter = current_function_parameter;
    previous_parameter = previous_parameter->next_parameter;
  } // end while loop

  const FunctionData *function_data = new_function_data(
      return_type, parameter_list_anchor.next_parameter, is_variadic);

  function_type->function_data = function_data;

  assert(get_current_token(lexer)->type == TokenType::RParen);
  expect_and_get_next_token(
      lexer, TokenType::RParen,
      "Parsing function parameter list, expected right parenthesis\n");

  return function_type;
}

// Declarations end with an init-declarator-list
//
// init-declarator: declarator
//                  declarator = initializer
//
//      a declaration may be e.g. int x = 3, y[5] = {0,} , z(), *ptr;
//      the declarators are x = 3, y[5] = {0,} , z(), and *ptr
//
//      the result of parsing a declarator is a new identifier
//      of a certain type: variable, function, array/ptr
//
// declarator: pointer(opt) direct-declarator
const Object *parse_declarator(Lexer *lexer, const Type *base_type,
                               Scope *scope) {
  // in `const int* const x;` we enter this function on the asterisk
  // in `int x;`              we enter on the x

  // return type is the type of the object this function returns
  // it can be mutated either by becoming the base type of a pointer/array
  // and/or by becoming the return type of a function
  const Type *return_type = base_type;

  // check for pointer type
  if (get_current_token(lexer)->type == TokenType::Asterisk) {
    return_type = parse_pointer(lexer, return_type);
  }

  // after checking for pointer types, a declarator needs to specify an
  // identifier
  const Token *identifier_token = get_current_token(lexer);
  const std::string identifier = identifier_token->string;

  expect_and_get_next_token(lexer, TokenType::Identifier,
                            "Parsing declarator, expected identifier name "
                            "after declaration specifiers and pointers");

  // next is the direct declarators, which we don't have an function for
  // a direct declarator begins with an identifier, followed by either array
  // dimensions or function parameter lists
  if (get_current_token(lexer)->type == TokenType::LParen)
    return_type = parse_parameter_list(lexer, return_type, scope);

  if (get_current_token(lexer)->type == TokenType::LBracket)
    return_type = parse_array_dimensions(lexer);

  return new_object(identifier, return_type);
}

// e.g. parse a const*
static DeclarationSpecifierFlags parse_type_qualifier_list(Lexer *lexer) {

  const Token *current_token = get_current_token(lexer);

  DeclarationSpecifierFlags declaration;
  declaration.flags = 0;

  while (token_is_type_qualifier(current_token)) {
    update_declaration_specifiers(current_token, &declaration);
    current_token = get_next_token(lexer);
  }

  return declaration;
}

// pointer is *type-qualifier-list(opt) followed by another ptr(opt)
// e.g. parse a:        int *const *volatile x;
// x is a volatile pointer to a const pointer to int
//
// the result of parsing a pointer is a new type of fundamental type pointer,
// pointing to a base type
//
// pointer: * type-qualifier-list(optional)
//          * type-qualifier-list(optional) pointer
const Type *parse_pointer(Lexer *lexer, const Type *base_type) {
  assert(get_current_token(lexer)->type == TokenType::Asterisk);

  const Type *current_base_type = base_type;

  // if we get an int **x, we want to return the pointer to the pointer to int
  while (get_current_token(lexer)->type == TokenType::Asterisk) {
    get_next_token(lexer);

    Type *pointer_type = new_type(FundamentalType::Pointer);

    DeclarationSpecifierFlags type_qualifiers =
        parse_type_qualifier_list(lexer);

    pointer_type->declaration_specifier_flags = type_qualifiers;
    pointer_type->pointed_type = current_base_type;
    current_base_type = pointer_type;
  }

  assert(base_type != current_base_type);
  return current_base_type;
}

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
//      direct-declarator (identifier-list(opt))
//          this is for old-style K&R function declarations

// 6.7.7 Type names
// type-name:
//   specifier-qualifier-list abstract-declarator(optional)
//      spec-qual-list is like const int
void parse_typename() {}

// specifier-qualifier-list:
//      specifier-qualifier-list(optional) type-specifiers/qualifier
DeclarationSpecifierFlags parse_specifier_qualifier_list(Lexer *lexer,
                                                         Scope *scope) {
  const Token *current_token = get_next_token(lexer);
  DeclarationSpecifierFlags declaration;

  while (token_is_type_specifier(current_token, scope) ||
         token_is_type_qualifier(current_token)) {

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

// 6.7.9 Initialization
//  initializers come from declaration = initializer
//  so the 5 in x = 5, or the {1,2} in x[2] = {1,2}

// initializer:
//      assignment-expression
//      { initializer-list }
//      { initializer-list, }
ASTNode *parse_initializer(Lexer *lexer) {

  // { initializer list }
  if (get_current_token(lexer)->type == TokenType::LBracket) {
    get_next_token(lexer);
    parse_initializer_list(lexer);

    // skip potential comma
    if (get_current_token(lexer)->type == TokenType::Comma)
      get_next_token(lexer);

    expect_and_get_next_token(
        lexer, TokenType::RBracket,
        "Expected RBracket after comma at end of initializer list\n");
  }

  // simple assignment expresion
  return parse_assignment_expression(lexer);
}

// initializer-list:
//      designation(optional) initializer
//      initializer-list, designation(optional) initializer
//
ASTNode *parse_initializer_list(Lexer *lexer) {

  // FIXME: struct initializers
  if (get_current_token(lexer)->type == TokenType::LBracket) {
    expect_and_get_next_token(lexer, TokenType::RBracket,
                              "Expected closing bracket in array designator");
  }

  // FIXME: array initializers
  if (get_current_token(lexer)->type == TokenType::Dot) {
    get_next_token(lexer);

    if (get_current_token(lexer)->type != TokenType::Identifier)
      error_token(lexer,
                  "Expected identifer name after '.' in intializer list");

    const std::string &identifier = get_current_token(lexer)->string;
    (void)identifier;
  }

  return parse_initializer(lexer);
}

// designation:
//      designator-list =

// designator-list
//      designator
//      designator-list designator

// designator
//      [constant-expression]
//      .identifier

// [const-expr] is for array types,  any nonnegative values allowed if size
// unspecified
// .identifier is for struct/unions, the identifier better be a member

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