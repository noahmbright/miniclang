#pragma once

#include "lexer.h"
#include <string>

struct Type;

struct Identifier {
  std::string name;
};

struct FunctionType {
  Identifier *parameter;
  bool is_variadic;
};

struct ArrayType {
  Type *base;
};

struct PointerType {
  Type *base;
};

struct Declaration {
  int flags;
};

struct AbstractType {

  union {
    FunctionType function_type;
    PointerType pointer_type;
    ArrayType array_type;
  } as;

  AbstractType *abstract_type;
};

enum class DataType {
  Void,
  Char,
  SignedChar,
  UnsignedChar,
  Short,
  UnsignedShort,
  Int,
  UnsignedInt,
  Long,
  UnsignedLong,
  LongLong,
  UnsignedLongLong,
  Float,
  Double,
  LongDouble,
  FloatComplex,
  DoubleComplex,
  LongDoubleComplex,
  Struct,
  Union,
  Enum,
  EnumeratedValue,
  TypedefName
};

// a type name is a list of type specifiers/qualifiers and an optional abstract
// declarator
// i.e., a const *[]
struct Type {
  DataType kind;
  bool is_atomic;
  AbstractType *abstract_type;
};

struct Function {};

struct Variable {};

// functions or variables
struct Object {
  std::string name;
  union {
    Variable variable;
    Function function;
  } as;
};

enum TypeModifierFlag {
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
DataType type_kind_from_declaration(Declaration *declaration);
AbstractType *new_abstract_type();
bool is_arithmetic_type(DataType t);
bool is_integer_type(DataType t);
bool is_floating_type(DataType t);
