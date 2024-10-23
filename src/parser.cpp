#include "parser.h"
#include "lexer.h"
#include "type.h"

#include <cassert>

void parse_direct_abstract_declarator(Lexer *lexer);
void parse_initializer(Lexer *lexer);
Object *parse_declarator(Lexer *lexer);
ASTNode *parse_init_declarator(Lexer *);
AbstractType *parse_abstract_declarator(Lexer *lexer);
Object *parse_direct_declarator(Lexer *lexer, Object *object = nullptr);
void parse_pointer(Lexer *);

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
  return new_object;
}

/*static bool variable_in_scope(std::string variable_name, Scope *scope) {

  for (Scope *current_scope = scope; current_scope != nullptr;
       current_scope = current_scope->parent_scope)

    if (current_scope->variables.contains(variable_name))
      return true;

  return false;
}*/

static bool typedef_name_in_scope(std::string type_name, Scope *scope) {

  for (Scope *current_scope = scope; current_scope != nullptr;
       current_scope = current_scope->parent_scope)

    if (current_scope->typedef_names.contains(type_name))
      return true;

  return false;
}

// parsing expressions
// this is where in the grammar operator precedence is defined
// the earlier in the grammar an operation is defined, the higher the
// precedence of that operation
//
// the approach here is like that of Chibicc, pure recursive descent
// clang uses operator precedence as well, see clang/lib/Parse/ParseExpr.cpp
//
// the challenge here is eliminating the left recursion from the grammar
// following 6.5.4 cast-exprs, each rule is either the next higher precedence
// rule, or a left recursive alternative, see e.g. multiplicative expressions
//
//

// 6.5.1 Primary expressions
// typical primary expressions are either identifiers or literals
// to motivate the type of node returned by a primary expression,
// take an add expression for example
//   +
//  / \
// x   y
// all told, we want to have a node for the add with lhs x and rhs y,
// so a primary expression node will return a node that holds
// raw data/identifiers that describe raw data

// primary expressions can be identifiers, constants, string literals,
// the simplified results of parsing a larger expression wrapped in parentheses,
// or a generic-selection

static int hex_char_to_int(char c) {

  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  assert(false && "hex_char_to_int got invalid hex digit");
}

static int decimal_to_int(char c) {
  assert(c >= '0' && c <= '9');
  return c - '0';
}

static int octal_to_int(char c) {
  assert(c >= '0' && c <= '7');
  return c - '0';
}

static int binary_to_int(char c) {
  assert(c >= '0' && c <= '1');
  return c - '0';
}

// FIXME add support for floats and doubles
static ASTNode *parse_number(Lexer *lexer) {

  Token *current_token = get_current_token(lexer);
  assert(current_token->type == TokenType::Number &&
         "Parsing number but initial token type is not number");

  // for hex/binary/octal numbers, get the base and and index to
  // start iterating through the string containing the numeric value
  // e.g., to skip the 0x in a hex constant
  int base = 10;
  int index = 0;
  const std::string number_string = current_token->string;
  int n = number_string.length();
  int (*char_to_int_function)(char) = decimal_to_int;

  char c0 = number_string[0];
  if (c0 == '0') {
    char c1 = number_string[1];
    if (c1 == 'x') {
      base = 16;
      index = 2;
      char_to_int_function = hex_char_to_int;
    } else if (c1 == 'b') {
      base = 2;
      index = 2;
      char_to_int_function = binary_to_int;
    } else {
      base = 8;
      index = 1;
      char_to_int_function = octal_to_int;
    }
  }

  ASTNode *number_node = new_ast_node(ASTNodeType::NumericConstant);

  switch ((get_current_token(lexer)->type)) {
  case TokenType::IntegerSuffixl:
  case TokenType::IntegerSuffixL: {

    long value = 0;
    for (; index < n; index++) {
      value = value * base + char_to_int_function(number_string[index]);
    }

    get_next_token(lexer);
    number_node->data_type = DataType::Long;
    number_node->data_as.long_data = value;
    return number_node;
  }

  case TokenType::IntegerSuffixu:
  case TokenType::IntegerSuffixU: {
    unsigned value = 0;
    for (; index < n; index++)
      value = value * base + char_to_int_function(number_string[index]);

    get_next_token(lexer);
    number_node->data_type = DataType::UnsignedInt;
    number_node->data_as.unsigned_int_data = value;
    return number_node;
  }

  case TokenType::IntegerSuffixll:
  case TokenType::IntegerSuffixLL: {
    long long value = 0;
    for (; index < n; index++)
      value = value * base + char_to_int_function(number_string[index]);

    get_next_token(lexer);
    number_node->data_type = DataType::LongLong;
    number_node->data_as.long_long_data = value;
    return number_node;
  }
  case TokenType::IntegerSuffixull:
  case TokenType::IntegerSuffixuLL:
  case TokenType::IntegerSuffixllu:
  case TokenType::IntegerSuffixLLu:
  case TokenType::IntegerSuffixUll:
  case TokenType::IntegerSuffixULL:
  case TokenType::IntegerSuffixllU:
  case TokenType::IntegerSuffixLLU: {
    unsigned long long value = 0;
    for (; index < n; index++)
      value = value * base + char_to_int_function(number_string[index]);

    get_next_token(lexer);
    number_node->data_type = DataType::UnsignedLongLong;
    number_node->data_as.unsigned_long_long_data = value;
    return number_node;
  }

  default: {
    int value = 0;
    for (; index < n; index++) {
      value = value * base + char_to_int_function(number_string[index]);
    }

    get_next_token(lexer);
    number_node->data_type = DataType::Int;
    number_node->data_as.int_data = value;
    return number_node;
  }
  }
}

// primary expressions
//      identifier
//          lvalues or function designator
//          a series of alphanumerics, normal names
//          enum constants are constants, but identified
//          by an identifier
//
//      constant - integer, float, char
//      string-literal
//      (expression)
//      generic-selection
ASTNode *parse_primary_expression(Lexer *lexer /*, Scope *scope*/) {

  switch (get_current_token(lexer)->type) {
    // FIXME: parse identifiers
    //      handle declarations next
    // variable, enum const, or function
  case TokenType::Identifier:

  case TokenType::Number:
    return parse_number(lexer);

  default:
    assert(false && "Default case in parse_primary_expression");
  }
}

// 6.5.2
// postfix expressions:
//       primary expression
//       postfix-expression [ expression ]
//       postfix-expression ( argument-expression-listopt )
//       postfix-expression . identifier
//       postfix-expression -> identifier
//       postfix-expression ++
//       postfix-expression --
//       ( type-name ) { initializer-list }
//       ( type-name ) { initializer-list , }
ASTNode *parse_postfix_expression(Lexer *lexer) {
  ASTNode *root = parse_primary_expression(lexer);

  // FIXME
  const Token *current_token = get_current_token(lexer);
  while (current_token->type == TokenType::LBracket ||
         current_token->type == TokenType::LParen ||
         current_token->type == TokenType::Dot ||
         current_token->type == TokenType::ArrowOperator ||
         current_token->type == TokenType::PlusPlus ||
         current_token->type == TokenType::MinusMinus) {
  }
  // FIXME: type name initializer list ones

  return root;
}

// 6.5.3
// unary expression:
//  postfix-expr
//  ++ unary-expr
//  -- unary-expr
//  unary-operator cast-expr
//  sizeof unary-expr
//  sizeof (typename)
//  _Alignof (typename)
static bool is_unary_operator(Token *token) {
  TokenType t = token->type;
  using enum TokenType;
  return t == Ampersand || t == Asterisk || t == Plus || t == Minus ||
         t == Tilde || t == Bang || t == PlusPlus || t == MinusMinus ||
         t == SizeOf; // t== Alignof;
}
ASTNode *parse_unary_expression(Lexer *lexer) {
  Token *current_token = get_current_token(lexer);
  // FIXME: Unary operators
  while (is_unary_operator(current_token)) {
  }

  ASTNode *root = parse_postfix_expression(lexer);
  return root;
}

// 6.5.4 cast-expr
//          unary-expr
//          (typename) cast-expr
ASTNode *parse_cast_expression(Lexer *lexer) {
  if (get_current_token(lexer)->type == TokenType::LParen) {
    // FIXME: Parse typename
    expect_and_get_next_token(lexer, TokenType::RParen,
                              "Type cast expected RParen");
  }

  ASTNode *root = parse_unary_expression(lexer);

  return root;
}

// hereafter, each binary operator and its precedence is defined through
// left-recursive productions
//
// unwinding the cast-expr grammar, a simple valid cast-expr is the const 1 or
// 2
//
// forward referencing the next rule for mult-exprs, a cast-expr is a valid
// mult-expr, so 1 or 2 is also a valid mult-expr. Therefore, we are justified
// in stopping if all we have is a cast-expr followed by no (* or / or %)
//
// using the second production, we could also have 1 * 2, and recurring from
// there, 1 * 2 / 3, so on and so forth
//
// What should the AST for 1 * 2 / 3 look like? It should evaluate from left to
// right and give
//          *
//         / \
//        1   operator /
//              / \
//             2   3
//
// we hit our Number token 1 which we want to give us the lhs of this
// ASTNode
//
// we advance, and we see the next token is a *, so we recursively call
// parse_multiplicative_expression again, and this should return us the rhs of
// the node from the first call -  this rhs will be the division node with
// lhs 2 and rhs 3
//
// What if we don't see a multiplicative operator? Then we don't have a bonafied
// multiplication node. That's fine, then we just pop up the information from
// the cast node parse we have to do anyway. The cast node probably doesn't even
// have a type cast either, it's most likely we'll have to propagate up to an
// identifier or something
//
// so we can transform this rule into a cast-expr followed by 0 or more (* or /
// or %) and another cast-expr. If we see one of the right operators, we return
// a node with rhs and lhs properly set. If not, we just return whatever the
// cast node gave us
//
// the other thing to consider is how this handles operator precedence. After
// mult-exprs, we have add-exprs. Citing PEMDAS, multiplicative expressions have
// higher precedence than addition, so 2 + 3 * 4 should give
//          +
//         / \
//        2   *
//           / \
//          3   4
//
// add-expr is defined in terms of mult-expr, so if we have a bonafide add-expr,
// we'll be setting an lhs and rhs, and in order to set those, we'll call
// mult-expr. If the mult-expr is a bonafide multiplication, it will return a
// node with a multiplication operator at it's root, and an lhs and rhs with its
// operands back to the add-expr
//
// finally, these "rule (operator rule)*" rules we'll implement can naturally be
// implemented either recursively or iteratively
// recursion is prettier, but can inflate your call stack
// since the call stack already has to trudge through 15 levels to get to a
// primary expression, we'll be nice to it and go iterative
//

// FIXME: When to do type checking/casting?
ASTNode *new_binary_expression_node(ASTNodeType type, ASTNode *lhs,
                                    ASTNode *rhs) {
  ASTNode *binary_ast_node = new_ast_node(type);
  binary_ast_node->lhs = lhs;
  binary_ast_node->rhs = rhs;

  return binary_ast_node;
}

// 6.5.5 mult-expr
//          cast-expr
//          mult-expr (* or / or %) cast-expr
//
// implement as:
//      mult-expr: cast-expr ((* or / or %) cast-expr)*
//
// how do we process the while loop? unless we see a *, then we just return the
// cast node. If we see a *, the node becomes a multiplication node with an LHS
// equal to the cast node we initially parse, and an RHS equal to the result of
// parsing a cast starting on the next node
ASTNode *parse_multiplicative_expression(Lexer *lexer) {
  // get lhs, root of this parse subtree is whatever the cast expr gives us
  ASTNode *root = parse_cast_expression(lexer);

  const Token *current_token = get_current_token(lexer);

  while (current_token->type == TokenType::Asterisk ||
         current_token->type == TokenType::ForwardSlash ||
         current_token->type == TokenType::Modulo) {

    switch (current_token->type) {
      // hit *, now the root should be the multiplication operation
    case TokenType::Asterisk:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Multiplication, root,
                                        parse_cast_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    case TokenType::ForwardSlash:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Division, root,
                                        parse_cast_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    case TokenType::Modulo:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Modulo, root,
                                        parse_cast_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    default:
      break;
    }
  }

  // get_next_token(lexer);
  return root;
}

// 6.5.6 add-expr
//          mult-expr
//          add-expr (+ or -) mult-expr
ASTNode *parse_additive_expression(Lexer *lexer) {
  ASTNode *root = parse_multiplicative_expression(lexer);

  const Token *current_token = get_current_token(lexer);

  while (current_token->type == TokenType::Plus ||
         current_token->type == TokenType::Minus) {

    switch (current_token->type) {
    case TokenType::Plus:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Addition, root,
                                        parse_multiplicative_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    case TokenType::Minus:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Subtraction, root,
                                        parse_multiplicative_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    default:
      break;
    }
  }

  return root;
}

// 6.5.7 shift-expr
//          add-expr
//          shift-expr (>> or <<) add-expr
ASTNode *parse_shift_expression(Lexer *lexer) {
  ASTNode *root = parse_additive_expression(lexer);

  const Token *current_token = get_current_token(lexer);
  while (current_token->type == TokenType::BitShiftLeft ||
         current_token->type == TokenType::BitShiftRight) {
    switch (current_token->type) {
    case TokenType::BitShiftLeft:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::BitShiftLeft, root,
                                        parse_additive_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    case TokenType::BitShiftRight:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::BitShiftRight, root,
                                        parse_additive_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    default:
      break;
    }
  }

  return root;
}

// 6.5.8 relational-expr
//          shift-expr
//          relational-expr (< or > or <= or >=) shift-expr
ASTNode *parse_relational_expression(Lexer *lexer) {
  ASTNode *root = parse_shift_expression(lexer);

  const Token *current_token = get_current_token(lexer);

  while (current_token->type == TokenType::LessThan ||
         current_token->type == TokenType::LessThanOrEqualTo ||
         current_token->type == TokenType::GreaterThan ||
         current_token->type == TokenType::GreaterThanOrEqualTo) {
    switch (current_token->type) {

    case TokenType::LessThan:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::LessThan, root,
                                        parse_shift_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    case TokenType::LessThanOrEqualTo:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::LessThanOrEqualTo, root,
                                        parse_shift_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    case TokenType::GreaterThan:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::GreaterThan, root,
                                        parse_shift_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    case TokenType::GreaterThanOrEqualTo:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::GreaterThanOrEqualTo, root,
                                        parse_shift_expression(lexer));

    default:
      break;
    }
  }

  return root;
}

// 6.5.9 equality-expr
//          relational-expr
//          equality-expr (== or !=) relational-expr
ASTNode *parse_equality_expression(Lexer *lexer) {
  ASTNode *root = parse_relational_expression(lexer);

  const Token *current_token = get_current_token(lexer);

  while (current_token->type == TokenType::BitShiftLeft ||
         current_token->type == TokenType::BitShiftRight) {

    switch (current_token->type) {
    case TokenType::BitShiftLeft:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::BitShiftLeft, root,
                                        parse_relational_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    case TokenType::BitShiftRight:
      get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::BitShiftRight, root,
                                        parse_relational_expression(lexer));
      current_token = get_current_token(lexer);
      break;

    default:
      break;
    }
  }

  return root;
}

// 6.5.10 and-expr
//          eq-expr
//          and-expr & eq-expr
ASTNode *parse_bitwise_and_expression(Lexer *lexer) {
  ASTNode *root = parse_equality_expression(lexer);

  // while (get_next_token(lexer)->type == TokenType::Ampersand) {
  // get_next_token(lexer);
  // root = new_binary_expression_node(ASTNodeType::BitwiseAnd, root,
  // parse_equality_expression(lexer));
  //}

  // get_next_token(lexer);
  return root;
}

// 6.5.11 xor-expr
//          and-expr
//          xor-expr ^ and-expr
ASTNode *parse_bitwise_xor_expression(Lexer *lexer) {

  ASTNode *root = parse_bitwise_and_expression(lexer);

  while (get_current_token(lexer)->type == TokenType::Caret) {
    get_next_token(lexer);
    root = new_binary_expression_node(ASTNodeType::BitwiseXor, root,
                                      parse_bitwise_and_expression(lexer));
  }

  return root;
}

// 6.5.12 or-expr
//          xor-expr
//          or-expr | xor-expr
ASTNode *parse_bitwise_or_expression(Lexer *lexer) {
  ASTNode *root = parse_bitwise_xor_expression(lexer);

  while (get_current_token(lexer)->type == TokenType::Pipe) {
    get_next_token(lexer);
    root = new_binary_expression_node(ASTNodeType::BitwiseOr, root,
                                      parse_bitwise_xor_expression(lexer));
  }

  return root;
}

// 6.5.13 logical-and-expr
//          inclusive-or-expr
//          local-and-expr && inclusive-or
ASTNode *parse_logical_and_expression(Lexer *lexer) {
  ASTNode *root = parse_bitwise_or_expression(lexer);

  while (get_current_token(lexer)->type == TokenType::LogicalAnd) {
    get_next_token(lexer);
    root = new_binary_expression_node(ASTNodeType::LogicalAnd, root,
                                      parse_bitwise_or_expression(lexer));
  }

  return root;
}

// 6.5.14 logical-or-expr
//          logical-and-expr
//          logical-or-expr || logical-and-expr
ASTNode *parse_logical_or_expression(Lexer *lexer) {
  ASTNode *root = parse_logical_and_expression(lexer);

  while (get_next_token(lexer)->type == TokenType::LogicalOr) {
    get_next_token(lexer);
    root = new_binary_expression_node(ASTNodeType::LogicalOr, root,
                                      parse_logical_and_expression(lexer));
  }

  return root;
}

// 6.5.15 conditional-expression
//          logical-or-expr
//          logical-or-expr ? expression : conditional-expression
// implement as
//          logical-or (? expression : logical-or)*
// the ast here looks like
//          ?
//       /  |   \
// or-expr if  else
ASTNode *parse_conditional_expression(Lexer *lexer) {
  ASTNode *root = parse_logical_or_expression(lexer);

  while (get_current_token(lexer)->type == TokenType::QuestionMark) {

    ASTNode *conditional_node =
        new_ast_node(ASTNodeType::ConditionalExpression);
    conditional_node->conditional = root;
    get_next_token(lexer);

    conditional_node->lhs = parse_expression(lexer);
    expect_and_get_next_token(
        lexer, TokenType::Colon,
        "Parsing ternary expression: expected ':' after expression");

    conditional_node->rhs = parse_logical_or_expression(lexer);

    return conditional_node;
  }

  return root;
}

// 6.5.16
// Assignment expression:
//      conditional expression
//      unary-expression assignment-operator assignment-expression
static bool is_assignment_operator(Token *token) {
  TokenType t = token->type;
  using enum TokenType;
  return (t == Equals || t == TimesEquals || t == DividedByEquals ||
          t == ModuloEquals || t == PlusEquals || t == MinusEquals ||
          t == BitShiftLeftEquals || t == BitShiftRightEquals ||
          t == BitwiseAndEquals || t == XorEquals || t == BitwiseOrEquals);
}

ASTNode *parse_assignment_expression(Lexer *lexer) {
  ASTNode *root = parse_conditional_expression(lexer);

  while (is_assignment_operator(get_current_token(lexer))) {
    // FIXME:
    break;
  }

  return root;
}

// 6.5.17 Comma operator
// expression:
//      assignment-expression
//      expression, assignment-expression
ASTNode *parse_expression(Lexer *lexer) {
  ASTNode *root = parse_assignment_expression(lexer);

  while (get_current_token(lexer)->type == TokenType::Comma) {
    // FIXME:
    break;
  }

  return root;
}

// 6.7 Declarations
// declaration: declaration-specifier init-declarator-list(optional)

// declaration specifiers are type qualifiers, storage class specifiers,
// type specifiers, function specifiers, and alignment specifiers
static bool token_is_type_qualifier(const Token *token) {
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

static bool token_is_storage_class_specifier(const Token *token) {
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

static bool token_is_alignment_specifier(const Token *token) {
  return token->type == TokenType::AlignAs;
}

static bool token_is_function_specifier(const Token *token) {
  using enum TokenType;
  return token->type == Inline || token->type == TokenType::NoReturn;
}

static bool token_is_type_specifier(const Token *token, Scope *scope) {
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
    return typedef_name_in_scope(token->string, scope);
  }
}

static bool token_is_declaration_specifier(const Token *token, Scope *scope) {
  return token_is_storage_class_specifier(token) ||
         token_is_type_specifier(token, scope) ||
         token_is_type_qualifier(token) || token_is_function_specifier(token) ||
         token_is_alignment_specifier(token);
}

Declaration parse_declaration_specifiers(Lexer *lexer, Scope *scope) {
  const Token *current_token = get_current_token(lexer);
  Declaration declaration;

  while (token_is_declaration_specifier(current_token, scope)) {
    update_declaration_specifiers(current_token, &declaration);
    current_token = get_next_token(lexer);
  }

  return declaration;
}

// declaration: declaration-specs init-declarator-list(opt)
//
// a declaration is e.g.
//      const int *x[] = {};
// the declaration specifiers are const int, and the init
// declarator list is the single declarator *x[] = {}
//
// the init declarator list is optional, so e.g. int;
// is a valid declaration, but it is just dead code
//
// parsing declarations is mostly done in 6.7.6 declarators
//
// after parsing a declaration, we make an AST node that will
// initialize the new objects that have been declarated
ASTNode *parse_declaration(Lexer *lexer, Scope *scope) {
  assert(token_is_declaration_specifier(get_current_token(lexer), scope) &&
         "parse_declaration: starting but current token is not a declaration "
         "specifier");

  ASTNode *ast_node = new_ast_node(ASTNodeType::Declaration);
  ASTNode *current_ast_node = ast_node;
  // an init-declarator-list is a list of init-declarators
  // init-declarators are either declarators or declarator = initializer
  // e.g. int x, y=5;
  // Declaration declaration = parse_declaration_specifiers(lexer, scope);

  while (get_current_token(lexer)->type != TokenType::Semicolon) {
    current_ast_node->next = parse_init_declarator(lexer);
    current_ast_node = current_ast_node->next;
  }

  return ast_node->next;
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
Object *parse_declarator(Lexer *lexer) {
  Token *current_token = get_current_token(lexer);
  if (current_token->type == TokenType::Asterisk) {
    get_next_token(lexer);
    parse_pointer(lexer);
  }

  // FIXME ??
  return parse_direct_declarator(lexer);
}

// e.g. parse a const*
static void parse_type_qualifier_list(Lexer *lexer) {
  const Token *current_token = get_current_token(lexer);

  Declaration declaration;
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

Object *parse_direct_declarator(Lexer *lexer, Object *object) {
  const Token *current_token = get_current_token(lexer);

  // parenthesis, parse another declarator
  if (current_token->type == TokenType::LParen) {
    get_next_token(lexer);
    parse_declarator(lexer);
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

Declaration parse_specifier_qualifier_list(Lexer *lexer, Scope *scope) {
  const Token *current_token = get_next_token(lexer);
  Declaration declaration;
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
// initializer:
//      assignment-expression
//      { initializer-list }
//      { initializer-list, }
void parse_initializer(Lexer *lexer) { (void)lexer; }

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
