#pragma once

#include "lexer.h"

struct Type;

struct FunctionParameter {
  Type const* parameter_type;
  std::string identifier;
  FunctionParameter* next_parameter;
};

// a function type is defined by its parameters and return type
struct FunctionData {
  Type const* return_type;
  FunctionParameter const* parameter_list;
  bool is_variadic;
};

struct DeclarationSpecifierFlags {
  int flags;
};

enum class FundamentalType {
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
  TypedefName,
  Pointer,
  Function
};

// these are defined so pointers/functions/arrays can point to a real C object
// as return/base types
extern Type const* const VoidType;
extern Type const* const CharType;
extern Type const* const SignedCharType;
extern Type const* const UnsignedCharType;
extern Type const* const ShortType;
extern Type const* const UnsignedShortType;
extern Type const* const IntType;
extern Type const* const UnsignedIntType;
extern Type const* const LongType;
extern Type const* const UnsignedLongType;
extern Type const* const LongLongType;
extern Type const* const UnsignedLongLongType;
extern Type const* const FloatType;
extern Type const* const DoubleType;
extern Type const* const LongDoubleType;
extern Type const* const FloatComplexType;
extern Type const* const DoubleComplexType;
extern Type const* const LongDoubleComplexType;
extern Type const* const BoolType;
extern Type const* const StructType;
extern Type const* const UnionType;
extern Type const* const EnumType;
extern Type const* const EnumeratedValueType;
extern Type const* const TypedefNameType;

// a type name is a list of type specifiers/qualifiers and an optional abstract
// declarator
// i.e., a const int *[]
struct Type {
  FunctionData const* function_data;
  Type const* pointed_type;
  FundamentalType fundamental_type;
  DeclarationSpecifierFlags declaration_specifier_flags;
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

void update_declaration_specifiers(Token const*, DeclarationSpecifierFlags*);
FundamentalType fundamental_type_from_declaration(DeclarationSpecifierFlags* declaration);

Type* new_type(FundamentalType, Type* = nullptr);
Type* fundamental_type(FundamentalType);

bool is_arithmetic_type(FundamentalType t);
bool is_integer_type(FundamentalType t);
bool is_floating_type(FundamentalType t);
Type const* get_fundamental_type_pointer(FundamentalType);
