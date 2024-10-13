#pragma once

#include "mini_string.h"

enum class TokenType {
  // compiler internals
  Eof,
  Identifier,
  Error,
  StringLiteral,

  // Punctuation
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
  Semicolon,
  Plus,
  Minus,
  ForwardSlash,
  BackSlash,
  GreaterThan,
  LessThan,
  SingleQuote,
  DoubleQuote,
  Equals,
  LessThanOrEqualTo,
  GreaterThanOrEqualTo,
  DoubleEquals,

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
  LogicalAndEquals,
  LogicalOrEquals,

  ArrowOperator,

  // control
  For,
  While,
  If,
  Else,
  Switch,
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
  //  typedef-name's also fall into type specifier category

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
  // _Atomic ( type name ) is a type specifier
  Atomic,

  // function specifiers
  Inline,
  NoReturn, // _Noreturn

  // Alignment Specifier
  AlignAs // _Alignas
};

struct Lexer {
  const char *current_filepath;
  const char *beginning_of_current_token;
  const char *current_location;
  unsigned line;
  unsigned column;
};

struct Token {
  TokenType type;
  String string;
  unsigned line;
  unsigned column;
};

Lexer new_lexer(const char *);
void tokenize_char_ptr(const char *);
Token make_token(Lexer *, TokenType);
Token next_token(Lexer *);
Token error_token(Lexer *, const char *);
void lexer_print_error_message(Lexer *lexer, const char *);
bool expect_token_type(Token token, TokenType type);
Token expect_and_skip(Lexer *lexer, TokenType type, const char *);
