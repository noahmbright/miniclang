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

struct DeclarationSpecifierFlags {
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
  Bool,
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
  Char = 1 << 1,
  Signed = 1 << 2,
  Unsigned = 1 << 3,
  Short = 1 << 4,
  // long can appear up to two times, so give it an extra bit
  Long = 1 << 5,
  LongTest = 1 << 6,
  Int = 1 << 7,
  Float = 1 << 8,
  Double = 1 << 9,
  Bool = 1 << 10,
  Complex = 1 << 11,

  // storage-class-specifier
  TypeDef = 1 << 12,
  Extern = 1 << 13,
  Static = 1 << 14,
  ThreadLocal = 1 << 15,
  Auto = 1 << 16,
  Register = 1 << 17,
  // type-qualifier
  Const = 1 << 18,
  Restrict = 1 << 19,
  Volatile = 1 << 20,
  Atomic = 1 << 21,
  // function-specifier
  Inline = 1 << 22,
  NoReturn = 1 << 23,
  // alignment-specifier
  Alignas = 1 << 24,

  // forgot the unbolded type-specifiers
  TypeDefName = 1 << 25,
  Struct = 1 << 26,
  Enum = 1 << 27
};

void update_declaration_specifiers(const Token *, DeclarationSpecifierFlags *);
DataType type_kind_from_declaration(DeclarationSpecifierFlags *declaration);

AbstractType *new_abstract_type();

bool is_arithmetic_type(DataType t);
bool is_integer_type(DataType t);
bool is_floating_type(DataType t);
