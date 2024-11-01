#include "parser.h"
#include <cassert>

void function(ExternalDeclaration* function_declaration)
{
  assert(function_declaration->type == ExternalDeclarationType::FunctionDefinition);
}
