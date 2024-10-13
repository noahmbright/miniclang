#include "type.h"
#include "lexer.h"
#include <cstdio>

static void set_declaration_flag(TypeSpecifierFlag flag,
                                 Declaration *declaration) {
  declaration->flags |= flag;
}

static void handle_storage_class_specifier_flag(TypeSpecifierFlag flag,
                                                Declaration *declaration) {
  // At most, one storage-class specifier may be given in the declaration
  // specifiers in a  declaration, except that _Thread_local may appear with
  // static or extern.

  // FIXME: fix block scope type issue

  using enum TypeSpecifierFlag;

  // any of the storage classes are already specified
  if (declaration->flags &&
      (TypeDef || Extern || Static || ThreadLocal || Auto | Register)) {
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

static void check_flag_set_and_update_if_not(TypeSpecifierFlag flag,
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
  // FIXME: Implement handle align as caveats
  //        probably want to do this in the parser

  set_declaration_flag(TypeSpecifierFlag::Alignas, declaration);
}

void update_declaration_specifiers(Token *token, Declaration *declaration) {
  // TODO: handle static_assert
  using enum TokenType;
  switch (token->type) {

  case Void:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Void, declaration);
    return;
  case Char:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Char, declaration);
    return;
  case Signed:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Signed, declaration);
    return;
  case Unsigned:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Unsigned, declaration);
    return;
  case Short:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Short, declaration);
    return;
  case Int:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Int, declaration);
    return;
  case Float:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Float, declaration);
    return;
  case Double:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Double, declaration);
    return;
  case Bool:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Bool, declaration);
    return;
  case Complex:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Complex, declaration);
    return;
  case TypeDefName:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::TypeDefName,
                                     declaration);
    return;
  case Struct:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Struct, declaration);
    return;
  case Enum:
    check_flag_set_and_update_if_not(TypeSpecifierFlag::Enum, declaration);
    return;

  case Long:
    if (declaration->flags & TypeSpecifierFlag::LongTest)
      fprintf(stderr, "Specifying too mang longs in type specification");
    else
      declaration->flags += TypeSpecifierFlag::Long;
    return;

    // storage class specifiers
  case Typedef:
    handle_storage_class_specifier_flag(TypeSpecifierFlag::TypeDef,
                                        declaration);
    return;
  case Extern:
    handle_storage_class_specifier_flag(TypeSpecifierFlag::Extern, declaration);
    return;
  case Static:
    handle_storage_class_specifier_flag(TypeSpecifierFlag::Static, declaration);
    return;
  case ThreadLocal:
    handle_storage_class_specifier_flag(TypeSpecifierFlag::ThreadLocal,
                                        declaration);
    return;
  case Auto:
    handle_storage_class_specifier_flag(TypeSpecifierFlag::Auto, declaration);
    return;
  case Register:
    handle_storage_class_specifier_flag(TypeSpecifierFlag::Register,
                                        declaration);
    return;

    // If the same qualifier appears more than once in the same
    // specifier-qualifier-list, either directly or via one or more typedefs,
    // the behavior is the same as if it appeared only once.
    // FIXME: Send a warning for duplicate flag
  case Const:
    set_declaration_flag(TypeSpecifierFlag::Const, declaration);
    return;
  case Restrict:
    set_declaration_flag(TypeSpecifierFlag::Restrict, declaration);
    return;
  case Volatile:
    set_declaration_flag(TypeSpecifierFlag::Volatile, declaration);
    return;
  case Atomic:
    set_declaration_flag(TypeSpecifierFlag::Atomic, declaration);
    return;

    // function specifiers
    // A function specifier may appear more than once; the behavior is the same
    // as if it appeared only once.
  case Inline:
    set_declaration_flag(TypeSpecifierFlag::Inline, declaration);
    return;
  case NoReturn:
    set_declaration_flag(TypeSpecifierFlag::NoReturn, declaration);
    return;

  case AlignAs:
    handle_align_as(declaration);
    return;

  default:
    fprintf(stderr, "Update declaration specifiers got a "
                    "non-declaration-specifier token type");
    return;
  }
}

Type type_from_declaration(Declaration *declaration) {
  // following ChibiCC's approach for handling the multiset
  // specification for type specifiers
  // augmenting by adding that e.g. long long long has too many longs
  Type type;

  // the first 13 types in TypeSpecifierFlag enum are type specifiers
  // 13 in hex is 0xd
  int declaration_type_as_int = declaration->flags & 0xd;

  using enum TypeSpecifierFlag;
  switch (declaration_type_as_int) {
  case 0:
    // typedef name? error?
    break;
  case Void:
  case Char:
  case Signed + Char:
  case Unsigned + Char:

  case Short:
  case Short + Signed:
  case Short + Int:
  case Signed + Short + Int:

  case Unsigned + Short:
  case Unsigned + Short + Int:

  case Int:
  case Signed:
  case Signed + Int:

  case Unsigned:
  case Unsigned + Int:

  case Long:
  case Signed + Long:
  case Long + Int:
  case Signed + Long + Int:

  case Unsigned + Long:
  case Unsigned + Long + Int:

  case Long + Long:
  case Signed + Long + Long:
  case Long + Long + Int:

  case Float:
  case Double:
  case Long + Double:
  case Bool:
  case Float + Complex:
  case Double + Complex:
  case Long + Double + Complex:
  }

  return type;
}
