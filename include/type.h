#pragma once

#include "lexer.h"
#include <string>

struct Type;

struct Identifier {
  std::string name;
};

struct FunctionParameter {
  const Type *parameter_type;
  FunctionParameter *next_parameter;
};

struct FunctionData {
  const Type *return_type;
  const FunctionParameter *parameter_list;
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

extern const Type *const VoidType;
extern const Type *const CharType;
extern const Type *const SignedCharType;
extern const Type *const UnsignedCharType;
extern const Type *const ShortType;
extern const Type *const UnsignedShortType;
extern const Type *const IntType;
extern const Type *const UnsignedIntType;
extern const Type *const LongType;
extern const Type *const UnsignedLongType;
extern const Type *const LongLongType;
extern const Type *const UnsignedLongLongType;
extern const Type *const FloatType;
extern const Type *const DoubleType;
extern const Type *const LongDoubleType;
extern const Type *const FloatComplexType;
extern const Type *const DoubleComplexType;
extern const Type *const LongDoubleComplexType;
extern const Type *const BoolType;
extern const Type *const StructType;
extern const Type *const UnionType;
extern const Type *const EnumType;
extern const Type *const EnumeratedValueType;
extern const Type *const TypedefNameType;

// a type name is a list of type specifiers/qualifiers and an optional abstract
// declarator
// i.e., a const *[]
struct Type {
  const FunctionData *function_data;
  const Type *pointed_type;
  FundamentalType fundamental_type;
  DeclarationSpecifierFlags declaration_specifier_flags;
};

// functions or variables
struct Object {
  std::string identifier;
  const Type *type;
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
FundamentalType
fundamental_type_from_declaration(DeclarationSpecifierFlags *declaration);

Type *new_type(FundamentalType, Type * = nullptr);
Type *fundamental_type(FundamentalType);

bool is_arithmetic_type(FundamentalType t);
bool is_integer_type(FundamentalType t);
bool is_floating_type(FundamentalType t);
const Type *get_fundamental_type_pointer(FundamentalType);
