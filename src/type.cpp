#include "type.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

static void set_declaration_flag(TypeModifierFlag flag,
                                 Declaration *declaration) {
  declaration->flags |= flag;
}

AbstractType *new_abstract_type() {
  AbstractType *abstract_type = (AbstractType *)malloc(sizeof(AbstractType));
  return abstract_type;
}

static void handle_storage_class_specifier_flag(TypeModifierFlag flag,
                                                Declaration *declaration) {
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

static void check_flag_set_and_update_if_not(TypeModifierFlag flag,
                                             Declaration *declaration) {
  if (declaration->flags & flag) {
    fprintf(stderr, "repeating type specifier");
    return;
  }

  set_declaration_flag(flag, declaration);
}

static void handle_align_as(Declaration *declaration) {
  // An alignment attribute shall not be specified in a declaration of a
  // typedef, or a bit-field, or a function, or a parameter, or an object
  // declared with the register storage-class specifier.
  //
  // FIXME: Implement handle alignas caveats
  //        probably want to do this in the parser

  set_declaration_flag(TypeModifierFlag::Alignas, declaration);
}

void update_declaration_specifiers(const Token *token,
                                   Declaration *declaration) {
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

DataType type_kind_from_declaration(Declaration *declaration) {
  // following ChibiCC's approach for handling the multiset
  // specification for type specifiers
  // augmenting by adding that e.g. long long long has too many longs

  // the first 13 types in TypeSpecifierFlag enum are type specifiers
  // 13 in hex is 0xd
  int declaration_type_as_int = declaration->flags & 0xd;

  switch (declaration_type_as_int) {
  case 0:
    // FIXME: typedef name? error?
    break;

  case TypeModifierFlag::Void:
    return DataType::Void;

  case TypeModifierFlag::Char:
  case TypeModifierFlag::Signed + TypeModifierFlag::Char:
  case TypeModifierFlag::Unsigned + TypeModifierFlag::Char:
    return DataType::Void;

  case TypeModifierFlag::Short:
  case TypeModifierFlag::Short + TypeModifierFlag::Signed:
  case TypeModifierFlag::Short + TypeModifierFlag::Int:
  case TypeModifierFlag::Signed + TypeModifierFlag::Short +
      TypeModifierFlag::Int:
    return DataType::Void;

  case TypeModifierFlag::Unsigned + TypeModifierFlag::Short:
  case TypeModifierFlag::Unsigned + TypeModifierFlag::Short +
      TypeModifierFlag::Int:
    return DataType::Void;

  case TypeModifierFlag::Int:
  case TypeModifierFlag::Signed:
  case TypeModifierFlag::Signed + TypeModifierFlag::Int:
    return DataType::Void;

  case TypeModifierFlag::Unsigned:
  case TypeModifierFlag::Unsigned + TypeModifierFlag::Int:
    return DataType::Void;

  case TypeModifierFlag::Long:
  case TypeModifierFlag::Signed + TypeModifierFlag::Long:
  case TypeModifierFlag::Long + TypeModifierFlag::Int:
  case TypeModifierFlag::Signed + TypeModifierFlag::Long +
      TypeModifierFlag::Int:
    return DataType::Void;

  case TypeModifierFlag::Unsigned + TypeModifierFlag::Long:
  case TypeModifierFlag::Unsigned + TypeModifierFlag::Long +
      TypeModifierFlag::Int:
    return DataType::Void;

  case TypeModifierFlag::Long + TypeModifierFlag::Long:
  case TypeModifierFlag::Signed + TypeModifierFlag::Long +
      TypeModifierFlag::Long:
  case TypeModifierFlag::Long + TypeModifierFlag::Long + TypeModifierFlag::Int:
    return DataType::Void;

  case TypeModifierFlag::Float:
    return DataType::Void;

  case TypeModifierFlag::Double:
    return DataType::Void;

  case TypeModifierFlag::Long + TypeModifierFlag::Double:
    return DataType::Void;

  case TypeModifierFlag::Bool:
    return DataType::Void;

  case TypeModifierFlag::Float + TypeModifierFlag::Complex:
    return DataType::Void;

  case TypeModifierFlag::Double + TypeModifierFlag::Complex:
    return DataType::Void;

  case TypeModifierFlag::Long + TypeModifierFlag::Double +
      TypeModifierFlag::Complex:
    return DataType::Void;
  }
  assert(false && "TypeKind from declaration UNREACHABLE");
}

bool is_integer_type(DataType t) {
  switch (t) {
  case DataType::SignedChar:
  case DataType::Char:
  case DataType::Int:
  case DataType::UnsignedInt:
  case DataType::Long:
  case DataType::UnsignedLong:
  case DataType::LongLong:
  case DataType::UnsignedLongLong:
  case DataType::Short:
  case DataType::UnsignedShort:
  case DataType::EnumeratedValue:
    return true;

  default:
    return false;
  }
}

bool is_floating_type(DataType t) {
  switch (t) {
  case DataType::Float:
  case DataType::Double:
  case DataType::LongDouble:
    return true;

  default:
    return false;
  }
}

bool is_arithmetic_type(DataType t) {
  return is_integer_type(t) || is_floating_type(t);
}
