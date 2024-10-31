#include "lexer.h"
#include "parser.h"
#include "type.h"

#include <cassert>

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
// 6.5.1 Primary expressions
// typical primary expressions are identifiers or literals, e.g. "5" or "x"
//
// to motivate the type of node returned by a primary expression,
// take an add expression for example
//   +
//  / \
// x   y
// all told, we want to have a node for the add with lhs x and rhs y,
// so a primary expression node will return a node that holds
// raw data/identifiers that describe raw data
//
// primary expressions can be identifiers, constants, string literals,
// the simplified results of parsing a larger expression wrapped in parentheses,
// or a generic-selection

static int hex_char_to_int(char c)
{

  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  assert(false && "hex_char_to_int got invalid hex digit");
}

static int decimal_to_int(char c)
{
  assert(c >= '0' && c <= '9');
  return c - '0';
}

static int octal_to_int(char c)
{
  assert(c >= '0' && c <= '7');
  return c - '0';
}

static int binary_to_int(char c)
{
  assert(c >= '0' && c <= '1');
  return c - '0';
}

// FIXME add support for floats and doubles
static ASTNode* parse_number(Lexer* lexer)
{

  Token* current_token = get_current_token(lexer);
  assert(current_token->type == TokenType::Number && "Parsing number but initial token type is not number");

  // for hex/binary/octal numbers, get the base and and index to
  // start iterating through the string containing the numeric value
  // e.g., to skip the 0x in a hex constant
  int base = 10;
  int index = 0;
  std::string const number_string = current_token->string;
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

  ASTNode* number_node = new_ast_node(ASTNodeType::NumericConstant);

  switch ((get_current_token(lexer)->type)) {
  case TokenType::IntegerSuffixl:
  case TokenType::IntegerSuffixL: {

    long value = 0;
    for (; index < n; index++) {
      value = value * base + char_to_int_function(number_string[index]);
    }

    get_next_token(lexer);
    number_node->data_type = FundamentalType::Long;
    number_node->data_as.long_data = value;
    return number_node;
  }

  case TokenType::IntegerSuffixu:
  case TokenType::IntegerSuffixU: {
    unsigned value = 0;
    for (; index < n; index++)
      value = value * base + char_to_int_function(number_string[index]);

    get_next_token(lexer);
    number_node->data_type = FundamentalType::UnsignedInt;
    number_node->data_as.unsigned_int_data = value;
    return number_node;
  }

  case TokenType::IntegerSuffixll:
  case TokenType::IntegerSuffixLL: {
    long long value = 0;
    for (; index < n; index++)
      value = value * base + char_to_int_function(number_string[index]);

    get_next_token(lexer);
    number_node->data_type = FundamentalType::LongLong;
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
    number_node->data_type = FundamentalType::UnsignedLongLong;
    number_node->data_as.unsigned_long_long_data = value;
    return number_node;
  }

  default: {
    int value = 0;
    for (; index < n; index++) {
      value = value * base + char_to_int_function(number_string[index]);
    }

    get_next_token(lexer);
    number_node->data_type = FundamentalType::Int;
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
ASTNode* parse_primary_expression(Lexer* lexer /*, Scope *scope*/)
{

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
ASTNode* parse_postfix_expression(Lexer* lexer)
{
  ASTNode* root = parse_primary_expression(lexer);

  // FIXME
  Token const* current_token = get_current_token(lexer);
  while (current_token->type == TokenType::LBracket || current_token->type == TokenType::LParen || current_token->type == TokenType::Dot || current_token->type == TokenType::ArrowOperator || current_token->type == TokenType::PlusPlus || current_token->type == TokenType::MinusMinus) {
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
static bool is_unary_operator(Token* token)
{
  TokenType t = token->type;
  using enum TokenType;
  return t == Ampersand || t == Asterisk || t == Plus || t == Minus || t == Tilde || t == Bang || t == PlusPlus || t == MinusMinus || t == SizeOf; // t== Alignof;
}
ASTNode* parse_unary_expression(Lexer* lexer)
{
  Token* current_token = get_current_token(lexer);
  // FIXME: Unary operators
  while (is_unary_operator(current_token)) {
  }

  ASTNode* root = parse_postfix_expression(lexer);
  return root;
}

// 6.5.4 cast-expr
//          unary-expr
//          (typename) cast-expr
ASTNode* parse_cast_expression(Lexer* lexer)
{
  if (get_current_token(lexer)->type == TokenType::LParen) {
    // FIXME: Parse typename
    expect_and_get_next_token(lexer, TokenType::RParen,
        "Type cast expected RParen");
  }

  ASTNode* root = parse_unary_expression(lexer);

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
ASTNode* new_binary_expression_node(ASTNodeType type, ASTNode* lhs,
    ASTNode* rhs)
{
  ASTNode* binary_ast_node = new_ast_node(type);
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
ASTNode* parse_multiplicative_expression(Lexer* lexer)
{
  // get lhs, root of this parse subtree is whatever the cast expr gives us
  ASTNode* root = parse_cast_expression(lexer);

  Token const* current_token = get_current_token(lexer);

  while (current_token->type == TokenType::Asterisk || current_token->type == TokenType::ForwardSlash || current_token->type == TokenType::Modulo) {

    switch (current_token->type) {
      // hit *, now the root should be the multiplication operation
    case TokenType::Asterisk:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Multiplication, root,
          parse_cast_expression(lexer));
      break;

    case TokenType::ForwardSlash:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Division, root,
          parse_cast_expression(lexer));
      break;

    case TokenType::Modulo:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Modulo, root,
          parse_cast_expression(lexer));
      break;

    default:
      break;
    }
  }

  return root;
}

// 6.5.6 add-expr
//          mult-expr
//          add-expr (+ or -) mult-expr
ASTNode* parse_additive_expression(Lexer* lexer)
{
  ASTNode* root = parse_multiplicative_expression(lexer);

  Token const* current_token = get_current_token(lexer);

  while (current_token->type == TokenType::Plus || current_token->type == TokenType::Minus) {

    switch (current_token->type) {
    case TokenType::Plus:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Addition, root,
          parse_multiplicative_expression(lexer));
      break;

    case TokenType::Minus:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::Subtraction, root,
          parse_multiplicative_expression(lexer));
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
ASTNode* parse_shift_expression(Lexer* lexer)
{
  ASTNode* root = parse_additive_expression(lexer);

  Token const* current_token = get_current_token(lexer);
  while (current_token->type == TokenType::BitShiftLeft || current_token->type == TokenType::BitShiftRight) {
    switch (current_token->type) {
    case TokenType::BitShiftLeft:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::BitShiftLeft, root,
          parse_additive_expression(lexer));
      break;

    case TokenType::BitShiftRight:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::BitShiftRight, root,
          parse_additive_expression(lexer));
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
ASTNode* parse_relational_expression(Lexer* lexer)
{
  ASTNode* root = parse_shift_expression(lexer);

  Token const* current_token = get_current_token(lexer);

  while (current_token->type == TokenType::LessThan || current_token->type == TokenType::LessThanOrEqualTo || current_token->type == TokenType::GreaterThan || current_token->type == TokenType::GreaterThanOrEqualTo) {
    switch (current_token->type) {

    case TokenType::LessThan:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::LessThan, root,
          parse_shift_expression(lexer));
      break;

    case TokenType::LessThanOrEqualTo:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::LessThanOrEqualTo, root,
          parse_shift_expression(lexer));
      break;

    case TokenType::GreaterThan:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::GreaterThan, root,
          parse_shift_expression(lexer));
      break;

    case TokenType::GreaterThanOrEqualTo:
      current_token = get_next_token(lexer);
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
ASTNode* parse_equality_expression(Lexer* lexer)
{
  ASTNode* root = parse_relational_expression(lexer);

  Token const* current_token = get_current_token(lexer);

  while (current_token->type == TokenType::BitShiftLeft || current_token->type == TokenType::BitShiftRight) {

    switch (current_token->type) {
    case TokenType::BitShiftLeft:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::BitShiftLeft, root,
          parse_relational_expression(lexer));
      break;

    case TokenType::BitShiftRight:
      current_token = get_next_token(lexer);
      root = new_binary_expression_node(ASTNodeType::BitShiftRight, root,
          parse_relational_expression(lexer));
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
ASTNode* parse_bitwise_and_expression(Lexer* lexer)
{
  ASTNode* root = parse_equality_expression(lexer);

  // while (get_next_token(lexer)->type == TokenType::Ampersand) {
  // get_next_token(lexer);
  // root = new_binary_expression_node(ASTNodeType::BitwiseAnd, root,
  // parse_equality_expression(lexer));
  //}

  return root;
}

// 6.5.11 xor-expr
//          and-expr
//          xor-expr ^ and-expr
ASTNode* parse_bitwise_xor_expression(Lexer* lexer)
{

  ASTNode* root = parse_bitwise_and_expression(lexer);

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
ASTNode* parse_bitwise_or_expression(Lexer* lexer)
{
  ASTNode* root = parse_bitwise_xor_expression(lexer);

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
ASTNode* parse_logical_and_expression(Lexer* lexer)
{
  ASTNode* root = parse_bitwise_or_expression(lexer);

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
ASTNode* parse_logical_or_expression(Lexer* lexer)
{
  ASTNode* root = parse_logical_and_expression(lexer);

  while (get_current_token(lexer)->type == TokenType::LogicalOr) {
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
ASTNode* parse_conditional_expression(Lexer* lexer)
{
  ASTNode* root = parse_logical_or_expression(lexer);

  while (get_current_token(lexer)->type == TokenType::QuestionMark) {

    ASTNode* conditional_node = new_ast_node(ASTNodeType::ConditionalExpression);
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
static bool is_assignment_operator(Token* token)
{
  TokenType t = token->type;
  return (t == TokenType::Equals || t == TokenType::TimesEquals || t == TokenType::DividedByEquals || t == TokenType::ModuloEquals || t == TokenType::PlusEquals || t == TokenType::MinusEquals || t == TokenType::BitShiftLeftEquals || t == TokenType::BitShiftRightEquals || t == TokenType::BitwiseAndEquals || t == TokenType::XorEquals || t == TokenType::BitwiseOrEquals);
}

ASTNode* parse_assignment_expression(Lexer* lexer)
{
  ASTNode* root = parse_conditional_expression(lexer);

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
ASTNode* parse_expression(Lexer* lexer)
{
  ASTNode* root = parse_assignment_expression(lexer);

  while (get_current_token(lexer)->type == TokenType::Comma) {
    // FIXME:
    break;
  }

  return root;
}
