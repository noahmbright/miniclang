#include "parser.h"
#include "type.h"

#include <cassert>
#include <string>

static void error_and_stop(char const* message)
{
  fprintf(stderr, "%s", message);
  exit(1);
}

static char const* type_to_string(Type const* type)
{
  switch (type->fundamental_type) {
  case FundamentalType::Void:
    return "void";

  case FundamentalType::Char:
  case FundamentalType::SignedChar:
  case FundamentalType::UnsignedChar:
    return "i8";

  case FundamentalType::Short:
  case FundamentalType::UnsignedShort:
    return "i16";

  case FundamentalType::Int:
  case FundamentalType::UnsignedInt:
  case FundamentalType::Long:
  case FundamentalType::UnsignedLong:
    return "i32";

  case FundamentalType::LongLong:
  case FundamentalType::UnsignedLongLong:
    return "i64";

  case FundamentalType::Float:
    return "float";
  case FundamentalType::Double:
    return "double";
  case FundamentalType::LongDouble:
    return "fp128";

  case FundamentalType::Bool:
    return "i1";

    // FIXME incomplete
  case FundamentalType::FloatComplex:
  case FundamentalType::DoubleComplex:
  case FundamentalType::LongDoubleComplex:
  case FundamentalType::Struct:
  case FundamentalType::Union:
  case FundamentalType::Enum:
  case FundamentalType::EnumeratedValue:
  case FundamentalType::TypedefName:
  case FundamentalType::Pointer:
  case FundamentalType::Function:
  default:
    assert(false && "emitting code for this type not implemented\n");
    return "";
  }
}

void emit_code_from_node(ASTNode const* ast_node, FILE* outfile)
{
  (void)ast_node;
  fprintf(outfile, "asdf\n");
}

// https://llvm.org/docs/LangRef.html#functions
// LLVM function definitions begin with the line
// define [linkage] [other stuff] <ResultType> @<FunctionName>([argument list]) [other stuff] { basic blocks }
// this function does that first line only
static void function_definition_signature(Object const* function_object, FILE* outfile)
{
  FunctionData const* function_data = function_object->type->function_data;
  fprintf(outfile, "define");

  if (function_data->return_type->declaration_specifier_flags.flags & TypeModifierFlag::Static)
    fprintf(outfile, " internal");

  // room for other stuff

  fprintf(outfile, " %s", type_to_string(function_data->return_type));
  fprintf(outfile, " @%s(", function_object->identifier.c_str());

  unsigned count = 0;
  for (FunctionParameter const* current_param = function_data->parameter_list; current_param; current_param = current_param->next_parameter) {
    if (current_param->identifier == "")
      error_and_stop("Function definition parameters must have identifiers");

    fprintf(outfile, "%s %%%d", type_to_string(current_param->parameter_type), count++);

    if (current_param->next_parameter)
      fprintf(outfile, ", ");
  }
  fprintf(outfile, ")");

  // room for other stuff maybe

  fprintf(outfile, "{\n");
}

// this gets appended to the function definition, which ends with {\n
// in C, the function body is a compound statment, so we just need to emit code corresponding to a compound statement
static void emit_function_body(ASTNode const* function_body_head_node, FILE* outfile)
{
  // begin the function definition with the "entry" basic block
  fprintf(outfile, "entry:\n");

  for (ASTNode const* current_node = function_body_head_node; current_node; current_node = current_node->next) {
    emit_code_from_node(current_node, outfile);
  }
}

static void emit_function_definition(ExternalDeclaration const* function_declaration, FILE* outfile)
{
  assert(function_declaration->type == ExternalDeclarationType::FunctionDefinition);
  ASTNode const* head_node = function_declaration->root_ast_node;
  Object const* function_object = head_node->object;

  function_definition_signature(function_object, outfile);
  emit_function_body(function_object->function_body, outfile);

  fprintf(outfile, "}\n");
}

void emit_llvm_from_translation_unit(ExternalDeclaration const* external_declaration, FILE* outfile)
{
  for (ExternalDeclaration const* current_declaration = external_declaration; current_declaration; current_declaration = current_declaration->next) {
    switch (current_declaration->type) {
    case ExternalDeclarationType::Declaration:
      assert(false && "codegen for declarations not implemented\n");
    case ExternalDeclarationType::FunctionDefinition:
      emit_function_definition(current_declaration, outfile);
    }
  }
}
