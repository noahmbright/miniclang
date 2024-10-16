#pragma once

#include "lexer.h"

struct Declaration {
  int flags;
};

struct Type {};

enum TypeSpecifierFlag {
  Error = 0,
  // type-specifier
  Void = 1,
  Char = 1 << 2,
  Signed = 1 << 3,
  Unsigned = 1 << 4,
  Short = 1 << 5,
  // long can appear up to two times, so give it an extra bit
  Long = 1 << 6,
  LongTest = 1 << 7,
  Int = 1 << 8,
  Float = 1 << 9,
  Double = 1 << 11,
  Bool = 1 << 12,
  Complex = 1 << 13,

  // storage-class-specifier
  TypeDef = 1 << 14,
  Extern = 1 << 15,
  Static = 1 << 16,
  ThreadLocal = 1 << 17,
  Auto = 1 << 18,
  Register = 1 << 19,
  // type-qualifier
  Const = 1 << 20,
  Restrict = 1 << 21,
  Volatile = 1 << 23,
  Atomic = 1 << 24,
  // function-specifier
  Inline = 1 << 25,
  NoReturn = 1 << 26,
  // alignment-specifier
  Alignas = 1 << 27,

  // forgot the unbolded type-specifiers
  TypeDefName = 1 << 28,
  Struct = 1 << 29,
  Enum = 1 << 30
};

void update_declaration(Token *, Declaration *);
