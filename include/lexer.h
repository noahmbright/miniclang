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
  Ellipsis = 28,

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
  Do = 53,
  While = 54,
  If = 55,
  Else = 56,
  Switch = 57,
  Default = 58,
  Case = 59,
  Continue = 60,
  Break = 61,
  GoTo = 62,
  Return = 63,

  SizeOf = 64,

  // type specifiers
  Int = 65,
  Float = 66,
  Double = 67,
  Unsigned = 68,
  Void = 69,
  Char = 70,
  Short = 71,
  Long = 72,
  Signed = 73,
  Bool = 74,
  Complex = 75, //_Complex
  Struct = 76,
  Union = 77,
  Enum = 78,
  // typedef-name's also fall into type specifier category
  TypeDefName = 79,

  // storage-class-specifier:
  Typedef = 80,
  Extern = 81,
  Static = 82,
  ThreadLocal = 83, // _Thread_local
  Auto = 84,
  Register = 85,

  // type qualifiers
  Const = 86,
  Restrict = 87,
  Volatile = 88,
  //  _Atomic ( type name ) is a type specifier
  Atomic = 89,

  // function specifiers
  Inline = 90,
  NoReturn = 91, // _Noreturn

  // Alignment Specifier
  AlignAs = 92, // _Alignas

  // bitwise equals
  XorEquals = 93,
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
Token *expect_and_get_next_token(Lexer *, TokenType, const char *);
