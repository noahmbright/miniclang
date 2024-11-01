#pragma once

#include <string>

enum class TokenType {
  // compiler internals
  NotStarted = -1,
  Eof = 0,
  Identifier,
  Error,
  StringLiteral,
  Number,

  // Punctuaion
  Comma,
  Dot,
  Bang,
  LParen,
  RParen,
  LBracket,
  RBracket,
  LBrace,
  RBrace,
  Asterisk,
  Semicolon, // 15
  Plus,
  Minus,
  ForwardSlash, // for division
  BackSlash,    /* \ */
  GreaterThan,
  LessThan,
  SingleQuote,
  DoubleQuote,
  Equals,
  LessThanOrEqualTo,
  GreaterThanOrEqualTo,
  DoubleEquals,
  Ellipsis,

  Caret,
  Ampersand,
  Pipe,
  BitShiftLeft,
  BitShiftRight,
  Tilde,
  PlusPlus,
  MinusMinus,

  LogicalAnd,
  LogicalOr,

  QuestionMark,
  Colon,

  Modulo,
  TimesEquals,
  PlusEquals,
  MinusEquals,
  DividedByEquals,
  BitShiftLeftEquals,
  BitShiftRightEquals,
  BitwiseAndEquals,
  BitwiseOrEquals,
  XorEquals,
  ModuloEquals,
  NotEquals,

  ArrowOperator,

  // control
  For,
  Do,
  While,
  If,
  Else,
  Switch,
  Default,
  Case,
  Continue,
  Break,
  GoTo,
  Return,

  SizeOf,

  // type specifiers
  Int,
  Float,
  Double,
  Unsigned,
  Void,
  Char,
  Short,
  Long,
  Signed,
  Bool,
  Complex, //_Complex
  Struct,
  Union,
  Enum,
  // typedef-name's also fall into type specifier category
  TypeDefName,

  // storage-class-specifier:
  Typedef,
  Extern,
  Static,
  ThreadLocal, // _Thread_local
  Auto,
  Register,

  // type qualifiers
  Const,
  Restrict,
  Volatile,
  //  _Atomic ( type name ) is a type specifier
  Atomic,

  // function specifiers
  Inline,
  NoReturn, // _Noreturn

  // Alignment Specifier
  AlignAs, // _Alignas

  // integer suffixes
  IntegerSuffixl,
  IntegerSuffixL,
  IntegerSuffixu,
  IntegerSuffixU,
  IntegerSuffixll,
  IntegerSuffixLL,
  IntegerSuffixull,
  IntegerSuffixuLL,
  IntegerSuffixllu,
  IntegerSuffixLLu,
  IntegerSuffixUll,
  IntegerSuffixULL,
  IntegerSuffixllU,
  IntegerSuffixLLU
};

struct Token {
  TokenType type;
  std::string string;
  unsigned line;
  unsigned column;
};

struct Lexer {
  char const* current_filepath;
  char const* beginning_of_current_token;
  char const* current_location;

  unsigned beginning_of_token_line;
  unsigned beginning_of_token_column;

  unsigned current_line;
  unsigned current_column;

  Token current_token;
};

Lexer new_lexer(char const*);
void tokenize_char_ptr(char const*);
Token make_token(TokenType, unsigned, unsigned, std::string = "");
bool token_equals(Token const*, Token const*);

Token* get_current_token(Lexer*);
Token const* get_next_token(Lexer*);
Token error_token(Lexer*, char const*);
void lexer_print_error_message(Lexer*, char const*);
Token const* expect_next_token_and_skip(Lexer* lexer, TokenType type, char const*);
Token const* expect_and_get_next_token(Lexer*, TokenType, char const*);

bool token_is_integer_suffix(Token*);
