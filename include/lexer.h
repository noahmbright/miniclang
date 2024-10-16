#pragma once

#include <string>

enum class TokenType {
  // compiler internals
  Eof = 0,
  Identifier = 1,
  Error = 2,
  StringLiteral = 3,
  Number = 4,

  // Punctuation
  Comma = 5,
  Dot = 6,
  Bang = 7,
  LParen = 8,
  RParen = 9,
  LBracket = 10,
  RBracket = 11,
  LBrace = 12,
  RBrace = 13,
  Asterisk = 14,
  Semicolon = 15, // 15
  Plus = 16,
  Minus = 17,
  ForwardSlash = 18, //
  BackSlash = 19,    /* \ */
  GreaterThan = 20,
  LessThan = 21,
  SingleQuote = 22,
  DoubleQuote = 23,
  Equals = 24,
  LessThanOrEqualTo = 25,
  GreaterThanOrEqualTo = 26,
  DoubleEquals = 27,
  Ellipses = 28,

  Caret = 29,
  Ampersand = 30,
  Pipe = 31,
  BitShiftLeft = 32,
  BitShiftRight = 33,
  Tilde = 34,
  PlusPlus = 35,
  MinusMinus = 36,

  LogicalAnd = 37,
  LogicalOr = 38,

  QuestionMark = 39,
  Colon = 40,

  Modulo = 41,
  TimesEquals = 42,
  PlusEquals = 43,
  MinusEquals = 44,
  DividedByEquals = 45,
  BitShiftLeftEquals = 46,
  BitShiftRightEquals = 47,
  LogicalAndEquals = 48,
  LogicalOrEquals = 49,
  NotEquals = 50,

  ArrowOperator = 51,

  // control
  For = 52,
  While = 53,
  If = 54,
  Else = 55,
  Switch = 56,
  Case = 57,
  Continue = 58,
  Break = 59,
  GoTo = 60,
  Return = 61,

  SizeOf = 62,

  // type specifiers
  Int = 63,
  Float = 64,
  Double = 65,
  Unsigned = 66,
  Void = 67,
  Char = 68,
  Short = 69,
  Long = 70,
  Signed = 71,
  Bool = 72,
  Complex = 73, //_Complex
  Struct = 74,
  Union = 75,
  Enum = 76,
  // typedef-name's also fall into type specifier category
  TypeDefName = 77,

  // storage-class-specifier:
  Typedef = 78,
  Extern = 79,
  Static = 80,
  ThreadLocal = 81, // _Thread_local
  Auto = 82,
  Register = 83,

  // type qualifiers
  Const = 84,
  Restrict = 85,
  Volatile = 86,
  //  _Atomic ( type name ) is a type specifier
  Atomic = 87,

  // function specifiers
  Inline = 88,
  NoReturn = 89, // _Noreturn

  // Alignment Specifier
  AlignAs = 90 // _Alignas
};

struct Token {
  TokenType type;
  std::string string;
  unsigned line;
  unsigned column;
};

struct Lexer {
  const char *current_filepath;
  const char *beginning_of_current_token;
  const char *current_location;

  unsigned beginning_of_token_line;
  unsigned beginning_of_token_column;

  unsigned current_line;
  unsigned current_column;

  Token current_token;
};

Lexer new_lexer(const char *);
void tokenize_char_ptr(const char *);
Token make_token(TokenType, unsigned, unsigned, std::string = "");
bool token_equals(Token *, Token *);

Token *get_current_token(Lexer *);
Token *get_next_token(Lexer *);
Token error_token(Lexer *, const char *);
void lexer_print_error_message(Lexer *, const char *);
bool expect_token_type(Token *, TokenType);
Token *expect_and_get_next_token(Lexer *, TokenType, const char *);
