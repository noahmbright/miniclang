#include "parser.h"
#include "type.h"

#include <cassert>

using IdentifierMap = std::unordered_map<std::string, unsigned>;

static void error_and_stop(char const* message)
{
  fprintf(stderr, "%s", message);
  exit(1);
}

static char const* print_numeric_literal_as_string(FILE* outfile, ASTNode const* ast_node)
{
  assert(ast_node->type == ASTNodeType::NumericConstant);
  switch (ast_node->data_type) {
  case FundamentalType::Int:
    fprintf(outfile, "%d", ast_node->data_as.int_data);
  default:
    assert(false);
  }
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

static void emit_code_from_node(ASTNode const* ast_node, FILE* outfile, IdentifierMap& identifier_map, unsigned* count)
{
  switch (ast_node->type) {

  case ASTNodeType::Void:
    return;

  case ASTNodeType::NumericConstant:
    print_numeric_literal_as_string(outfile, ast_node);
    return;

  case ASTNodeType::VariableReference:
    if (!identifier_map.contains(ast_node->referenced_variable))
      error_and_stop("Local variable not found in this identifier map\n");

    fprintf(outfile, "%%%d", identifier_map.at(ast_node->referenced_variable));
    return;

  case ASTNodeType::Declaration: {
    // a declaration is a series of "int x = 3"s or whatever
    // this requires us to put these new variables on the stack in accord with their type
    // then potentially initialize them
    //
    // variables are put on the LLVM stack using the alloca instruction
    // https://www.llvm.org/docs/LangRef.html#alloca-instruction
    //
    // alloca returns a pointer to the requested type, then the initialization can be done using loads and stores
    // https://www.llvm.org/docs/LangRef.html#store-instruction
    // a store's semantics are, in short, "store <type> <value>, ptr <ptr>"
    Object* current_object = ast_node->object;
    assert(current_object && "Emitting code for declaration with null object");

    identifier_map[current_object->identifier] = *count;
    fprintf(outfile, "  %%%u = alloca %s\n", *count++, type_to_string(current_object->type));

    // node has an initializer
    if (ast_node->rhs) { }
  }
    return;

  case ASTNodeType::Return:
    // FIXME what register to return?
    assert(ast_node->scope->return_type && "codegen for return statement with no return type");
    fprintf(outfile, "  ret %s %%%d\n", type_to_string(ast_node->scope->return_type), 0);
    return;
  case ASTNodeType::Multiplication:
  case ASTNodeType::Division:
  case ASTNodeType::Modulo:
  case ASTNodeType::Addition:
  // https://www.llvm.org/docs/LangRef.html#add-instruction
  // FIXME: Worry about wrap around
  case ASTNodeType::Subtraction:
  case ASTNodeType::BitShiftLeft:
  case ASTNodeType::BitShiftRight:
  case ASTNodeType::GreaterThan:
  case ASTNodeType::GreaterThanOrEqualTo:
  case ASTNodeType::LessThan:
  case ASTNodeType::LessThanOrEqualTo:
  case ASTNodeType::EqualityComparison:
  case ASTNodeType::InequalityComparison:
  case ASTNodeType::BitwiseAnd:
  case ASTNodeType::BitwiseXor:
  case ASTNodeType::BitwiseOr:
  case ASTNodeType::LogicalAnd:
  case ASTNodeType::LogicalOr:
  case ASTNodeType::ConditionalExpression:
  case ASTNodeType::Assignment:
  case ASTNodeType::If:
  case ASTNodeType::Switch:
  case ASTNodeType::For:
    assert(false && "emitting code not implemented");
  }
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
static void emit_function_body(Object const* function_object, FILE* outfile)
{
  assert(function_object->function_body);
  assert(function_object->type->function_data->return_type);

  // begin the function definition with the "entry" basic block
  fprintf(outfile, "entry:\n");

  IdentifierMap identifier_map;
  unsigned count = 0;
  for (FunctionParameter const* current_param = function_object->type->function_data->parameter_list; current_param != nullptr;
       current_param = current_param->next_parameter)
    identifier_map[current_param->identifier] = count++;
  printf("emittinf body\n");

  for (ASTNode const* current_ast_node = function_object->function_body; current_ast_node; current_ast_node = current_ast_node->next) {
    emit_code_from_node(current_ast_node, outfile, identifier_map, &count);
  }
}

static void emit_function_definition(ExternalDeclaration const* function_declaration, FILE* outfile)
{
  assert(function_declaration->type == ExternalDeclarationType::FunctionDefinition);
  ASTNode const* head_node = function_declaration->root_ast_node;
  Object const* function_object = head_node->object;

  function_definition_signature(function_object, outfile);
  emit_function_body(function_object, outfile);

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
