#include "type.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

static void set_declaration_flag(TypeModifierFlag flag,
                                 DeclarationSpecifierFlags *declaration) {
  declaration->flags |= flag;
}

Type *new_type(FundamentalType fundamental_type, Type *pointed_type) {
  Type *new_type = (Type *)malloc(sizeof(Type));

  new_type->function_data = nullptr;
  new_type->pointed_type = pointed_type;
  new_type->fundamental_type = fundamental_type;
  new_type->declaration_specifier_flags.flags = 0;

  return new_type;
}

static void
handle_storage_class_specifier_flag(TypeModifierFlag flag,
                                    DeclarationSpecifierFlags *declaration) {
  // At most, one storage-class specifier may be given in the declaration
  // specifiers in a  declaration, except that _Thread_local may appear with
  // static or extern.

  // FIXME: fix block scope type issue

  using enum TypeModifierFlag;

  // any of the storage classes are already specified
  if (declaration->flags &
      (TypeDef | Extern | Static | ThreadLocal | Auto | Register)) {
    bool new_flag_is_thread_local = (flag == ThreadLocal);
    bool new_flag_is_extern_or_static = (flag == Static || flag == Extern);

    bool set_flag_is_thread_local = (declaration->flags & ThreadLocal);
    bool set_flag_is_extern_or_static =
        (declaration->flags == Static || declaration->flags == Extern);

    bool new_is_thread_and_set_is_extern_or_static =
        new_flag_is_thread_local && set_flag_is_extern_or_static;
    bool new_is_extern_or_static_and_set_is_thread =
        new_flag_is_extern_or_static && set_flag_is_thread_local;

    bool can_set_new_flag = new_is_extern_or_static_and_set_is_thread ||
                            new_is_thread_and_set_is_extern_or_static;

    if (!can_set_new_flag) {
      fprintf(stderr, "Setting a storage class specifier a second time");
      return;
    }
  }

  set_declaration_flag(flag, declaration);
}

static void
check_flag_set_and_update_if_not(TypeModifierFlag flag,
                                 DeclarationSpecifierFlags *declaration) {
  if (declaration->flags & flag) {
    fprintf(stderr, "repeating type specifier");
    return;
  }

  set_declaration_flag(flag, declaration);
}

static void handle_align_as(DeclarationSpecifierFlags *declaration) {
  // An alignment attribute shall not be specified in a declaration of a
  // typedef, or a bit-field, or a function, or a parameter, or an object
  // declared with the register storage-class specifier.
  //
  // FIXME: Implement handle alignas caveats
  //        probably want to do this in the parser

  set_declaration_flag(TypeModifierFlag::Alignas, declaration);
}

void update_declaration_specifiers(const Token *token,
                                   DeclarationSpecifierFlags *declaration) {
  // TODO: handle static_assert
  switch (token->type) {

    // type specifiers
  case TokenType::Void:
    check_flag_set_and_update_if_not(TypeModifierFlag::Void, declaration);
    return;
  case TokenType::Char:
    check_flag_set_and_update_if_not(TypeModifierFlag::Char, declaration);
    return;
  case TokenType::Signed:
    check_flag_set_and_update_if_not(TypeModifierFlag::Signed, declaration);
    return;
  case TokenType::Unsigned:
    check_flag_set_and_update_if_not(TypeModifierFlag::Unsigned, declaration);
    return;
  case TokenType::Short:
    check_flag_set_and_update_if_not(TypeModifierFlag::Short, declaration);
    return;
  case TokenType::Int:
    check_flag_set_and_update_if_not(TypeModifierFlag::Int, declaration);
    return;
  case TokenType::Float:
    check_flag_set_and_update_if_not(TypeModifierFlag::Float, declaration);
    return;
  case TokenType::Double:
    check_flag_set_and_update_if_not(TypeModifierFlag::Double, declaration);
    return;
  case TokenType::Bool:
    check_flag_set_and_update_if_not(TypeModifierFlag::Bool, declaration);
    return;
  case TokenType::Complex:
    check_flag_set_and_update_if_not(TypeModifierFlag::Complex, declaration);
    return;
  case TokenType::TypeDefName:
    check_flag_set_and_update_if_not(TypeModifierFlag::TypeDefName,
                                     declaration);
    return;
  case TokenType::Struct:
    check_flag_set_and_update_if_not(TypeModifierFlag::Struct, declaration);
    return;
  case TokenType::Enum:
    check_flag_set_and_update_if_not(TypeModifierFlag::Enum, declaration);
    return;

  case TokenType::Long:
    if (declaration->flags & TypeModifierFlag::LongTest)
      fprintf(stderr, "Specifying too mang longs in type specification");
    else
      declaration->flags += TypeModifierFlag::Long;
    return;

    // storage class specifiers
  case TokenType::Typedef:
    handle_storage_class_specifier_flag(TypeModifierFlag::TypeDef, declaration);
    return;
  case TokenType::Extern:
    handle_storage_class_specifier_flag(TypeModifierFlag::Extern, declaration);
    return;
  case TokenType::Static:
    handle_storage_class_specifier_flag(TypeModifierFlag::Static, declaration);
    return;
  case TokenType::ThreadLocal:
    handle_storage_class_specifier_flag(TypeModifierFlag::ThreadLocal,
                                        declaration);
    return;
  case TokenType::Auto:
    handle_storage_class_specifier_flag(TypeModifierFlag::Auto, declaration);
    return;
  case TokenType::Register:
    handle_storage_class_specifier_flag(TypeModifierFlag::Register,
                                        declaration);
    return;

    // If the same qualifier appears more than once in the same
    // specifier-qualifier-list, either directly or via one or more typedefs,
    // the behavior is the same as if it appeared only once.
    // FIXME: Send a warning for duplicate flag
  case TokenType::Const:
    set_declaration_flag(TypeModifierFlag::Const, declaration);
    return;
  case TokenType::Restrict:
    set_declaration_flag(TypeModifierFlag::Restrict, declaration);
    return;
  case TokenType::Volatile:
    set_declaration_flag(TypeModifierFlag::Volatile, declaration);
    return;
  case TokenType::Atomic:
    set_declaration_flag(TypeModifierFlag::Atomic, declaration);
    return;

    // function specifiers
    // A function specifier may appear more than once; the behavior is the same
    // as if it appeared only once.
  case TokenType::Inline:
    set_declaration_flag(TypeModifierFlag::Inline, declaration);
    return;
  case TokenType::NoReturn:
    set_declaration_flag(TypeModifierFlag::NoReturn, declaration);
    return;

  case TokenType::AlignAs:
    handle_align_as(declaration);
    return;

  default:
    fprintf(stderr, "Update declaration specifiers got a "
                    "non-declaration-specifier token type");
    return;
  }
}

FundamentalType
fundamental_type_from_declaration(DeclarationSpecifierFlags *declaration) {
  // following ChibiCC's approach for handling the multiset
  // specification for type specifiers
  // augmenting by adding that e.g. long long long has too many longs

  // the first 13 types in TypeSpecifierFlag enum are type specifiers
  // 13 consecutive 1s in hex is 0x1fff
  int declaration_type_as_int = declaration->flags & 0xfff;

  switch (declaration_type_as_int) {
  case 0:
    // FIXME: typedef name? error?
    break;

  case TypeModifierFlag::Void:
    return FundamentalType::Void;

    // fixme?
  case TypeModifierFlag::Char:
  case TypeModifierFlag::Signed + TypeModifierFlag::Char:
  case TypeModifierFlag::Unsigned + TypeModifierFlag::Char:
    return FundamentalType::Char;

  case TypeModifierFlag::Short:
  case TypeModifierFlag::Short + TypeModifierFlag::Signed:
  case TypeModifierFlag::Short + TypeModifierFlag::Int:
  case TypeModifierFlag::Signed + TypeModifierFlag::Short +
      TypeModifierFlag::Int:
    return FundamentalType::Short;

  case TypeModifierFlag::Unsigned + TypeModifierFlag::Short:
  case TypeModifierFlag::Unsigned + TypeModifierFlag::Short +
      TypeModifierFlag::Int:
    return FundamentalType::UnsignedShort;

  case TypeModifierFlag::Int:
  case TypeModifierFlag::Signed:
  case TypeModifierFlag::Signed + TypeModifierFlag::Int:
    return FundamentalType::Int;

  case TypeModifierFlag::Unsigned:
  case TypeModifierFlag::Unsigned + TypeModifierFlag::Int:
    return FundamentalType::UnsignedInt;

  case TypeModifierFlag::Long:
  case TypeModifierFlag::Signed + TypeModifierFlag::Long:
  case TypeModifierFlag::Long + TypeModifierFlag::Int:
  case TypeModifierFlag::Signed + TypeModifierFlag::Long +
      TypeModifierFlag::Int:
    return FundamentalType::Long;

  case TypeModifierFlag::Unsigned + TypeModifierFlag::Long:
  case TypeModifierFlag::Unsigned + TypeModifierFlag::Long +
      TypeModifierFlag::Int:
    return FundamentalType::UnsignedLong;

  case TypeModifierFlag::Long + TypeModifierFlag::Long:
  case TypeModifierFlag::Signed + TypeModifierFlag::Long +
      TypeModifierFlag::Long:
  case TypeModifierFlag::Long + TypeModifierFlag::Long + TypeModifierFlag::Int:
    return FundamentalType::LongLong;

  case TypeModifierFlag::Float:
    return FundamentalType::Float;

  case TypeModifierFlag::Double:
    return FundamentalType::Double;

  case TypeModifierFlag::Long + TypeModifierFlag::Double:
    return FundamentalType::LongDouble;

  case TypeModifierFlag::Bool:
    return FundamentalType::Bool;

  case TypeModifierFlag::Float + TypeModifierFlag::Complex:
    return FundamentalType::FloatComplex;

  case TypeModifierFlag::Double + TypeModifierFlag::Complex:
    return FundamentalType::DoubleComplex;

  case TypeModifierFlag::Long + TypeModifierFlag::Double +
      TypeModifierFlag::Complex:
    return FundamentalType::LongDoubleComplex;
  }
  assert(false && "TypeKind from declaration UNREACHABLE");
}

bool is_integer_type(FundamentalType t) {
  switch (t) {
  case FundamentalType::SignedChar:
  case FundamentalType::Char:
  case FundamentalType::Int:
  case FundamentalType::UnsignedInt:
  case FundamentalType::Long:
  case FundamentalType::UnsignedLong:
  case FundamentalType::LongLong:
  case FundamentalType::UnsignedLongLong:
  case FundamentalType::Short:
  case FundamentalType::UnsignedShort:
  case FundamentalType::EnumeratedValue:
    return true;

  default:
    return false;
  }
}

bool is_floating_type(FundamentalType t) {
  switch (t) {
  case FundamentalType::Float:
  case FundamentalType::Double:
  case FundamentalType::LongDouble:
    return true;

  default:
    return false;
  }
}

bool is_arithmetic_type(FundamentalType t) {
  return is_integer_type(t) || is_floating_type(t);
}

extern const Type *const VoidType = new_type(FundamentalType::Void);
extern const Type *const CharType = new_type(FundamentalType::Char);
extern const Type *const SignedCharType = new_type(FundamentalType::SignedChar);
extern const Type *const UnsignedCharType =
    new_type(FundamentalType::UnsignedChar);
extern const Type *const ShortType = new_type(FundamentalType::Short);
extern const Type *const UnsignedShortType =
    new_type(FundamentalType::UnsignedShort);
extern const Type *const IntType = new_type(FundamentalType::Int);
extern const Type *const UnsignedIntType =
    new_type(FundamentalType::UnsignedInt);
extern const Type *const LongType = new_type(FundamentalType::Long);
extern const Type *const UnsignedLongType =
    new_type(FundamentalType::UnsignedLong);
extern const Type *const LongLongType = new_type(FundamentalType::LongLong);
extern const Type *const UnsignedLongLongType =
    new_type(FundamentalType::UnsignedLongLong);
extern const Type *const FloatType = new_type(FundamentalType::Float);
extern const Type *const DoubleType = new_type(FundamentalType::Double);
extern const Type *const LongDoubleType = new_type(FundamentalType::LongDouble);
extern const Type *const FloatComplexType =
    new_type(FundamentalType::FloatComplex);
extern const Type *const DoubleComplexType =
    new_type(FundamentalType::DoubleComplex);
extern const Type *const LongDoubleComplexType =
    new_type(FundamentalType::LongDoubleComplex);
extern const Type *const BoolType = new_type(FundamentalType::Bool);
extern const Type *const StructType = new_type(FundamentalType::Struct);
extern const Type *const UnionType = new_type(FundamentalType::Union);
extern const Type *const EnumType = new_type(FundamentalType::Enum);
extern const Type *const EnumeratedValueType =
    new_type(FundamentalType::EnumeratedValue);
extern const Type *const TypedefNameType =
    new_type(FundamentalType::TypedefName);

const Type *get_fundamental_type_pointer(FundamentalType type) {
  switch (type) {
  case FundamentalType::Void:
    return VoidType;
  case FundamentalType::Char:
    return CharType;
  case FundamentalType::SignedChar:
    return SignedCharType;
  case FundamentalType::UnsignedChar:
    return UnsignedCharType;
  case FundamentalType::Short:
    return ShortType;
  case FundamentalType::UnsignedShort:
    return UnsignedShortType;
  case FundamentalType::Int:
    return IntType;
  case FundamentalType::UnsignedInt:
    return UnsignedIntType;
  case FundamentalType::Long:
    return LongType;
  case FundamentalType::UnsignedLong:
    return UnsignedLongType;
  case FundamentalType::LongLong:
    return LongLongType;
  case FundamentalType::UnsignedLongLong:
    return UnsignedLongLongType;
  case FundamentalType::Float:
    return FloatType;
  case FundamentalType::Double:
    return DoubleType;
  case FundamentalType::LongDouble:
    return LongDoubleType;
  case FundamentalType::FloatComplex:
    return FloatComplexType;
  case FundamentalType::DoubleComplex:
    return DoubleComplexType;
  case FundamentalType::LongDoubleComplex:
    return LongDoubleComplexType;
  case FundamentalType::Bool:
    return BoolType;
  case FundamentalType::Struct:
    return StructType;
  case FundamentalType::Union:
    return UnionType;
  case FundamentalType::Enum:
    return EnumType;
  case FundamentalType::EnumeratedValue:
    return EnumeratedValueType;
  case FundamentalType::TypedefName:
    return TypedefNameType;
  case FundamentalType::Pointer:
  case FundamentalType::Function:
    return nullptr;
  }
}
