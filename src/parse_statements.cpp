#include "lexer.h"
#include "parser.h"
#include <cassert>

ASTNode *parse_compound_statement(Lexer *, Scope *);
ASTNode *parse_jump_statement(Lexer *);
ASTNode *parse_expression_statement(Lexer *);
ASTNode *parse_iteration_statement(Lexer *);
ASTNode *parse_selection_statement(Lexer *);
ASTNode *parse_labeled_statement(Lexer *);

// 6.8 Statements
//      labeled statement
//      compound statement
//      expression statement
//      selection statement
//      iteration statement
//      jump statement
ASTNode *parse_statement(Lexer *lexer, Scope *scope) {
  ASTNode *ast_node = new_ast_node(ASTNodeType::Void);
  switch (get_current_token(lexer)->type) {

  case TokenType::Identifier:
  case TokenType::Case:
  case TokenType::Default:
    return parse_labeled_statement(lexer);

  case TokenType::LBrace:
    return parse_compound_statement(lexer, scope);

  case TokenType::If:
  case TokenType::Switch:
    return parse_selection_statement(lexer);

  case TokenType::While:
  case TokenType::For:
  case TokenType::Do:
    return parse_iteration_statement(lexer);

  case TokenType::GoTo:
  case TokenType::Continue:
  case TokenType::Break:
  case TokenType::Return:
    return parse_jump_statement(lexer);

  default:
    return parse_expression_statement(lexer);
  }

  return ast_node;
}

// labeled statements
//      identifier : statement for use with goto
//      case const-expression : statement
//      default : statement
ASTNode *parse_labeled_statement(Lexer *lexer) {
  ASTNode *ast_node = new_ast_node(ASTNodeType::Void);
  get_current_token(lexer);
  return ast_node;
}

// compound statement are blocks of declarations and other statements wrapped in
// {}, for use in basically everything, e.g. for loops
//
// compound-statement: ( declaration | statement )*
ASTNode *parse_compound_statement(Lexer *lexer, Scope *scope) {
  assert(get_current_token(lexer)->type == TokenType::LBrace);
  get_next_token(lexer);

  ASTNode ast_node_anchor;
  ast_node_anchor.next = nullptr;
  ASTNode *previous_ast_node = &ast_node_anchor;

  while (get_current_token(lexer)->type != TokenType::RBrace) {

    ASTNode *current_ast_node;
    if (token_is_declaration_specifier(get_current_token(lexer), scope))
      current_ast_node = parse_declaration(lexer, scope);
    else
      current_ast_node = parse_statement(lexer, scope);

    previous_ast_node->next = current_ast_node;
    previous_ast_node = previous_ast_node->next;
  }

  assert(get_current_token(lexer)->type == TokenType::RBrace);
  get_next_token(lexer);
  return ast_node_anchor.next;
}

// expression statements are expr(opt);
ASTNode *parse_expression_statement(Lexer *lexer) {
  ASTNode *ast_node = new_ast_node(ASTNodeType::Void);
  get_current_token(lexer);
  return ast_node;
}

// selection statements are ifs/switches
ASTNode *parse_selection_statement(Lexer *lexer) {
  ASTNode *ast_node = new_ast_node(ASTNodeType::Void);
  get_current_token(lexer);
  return ast_node;
}

// iteration statements are (do) while and for
ASTNode *parse_iteration_statement(Lexer *lexer) {
  ASTNode *ast_node = new_ast_node(ASTNodeType::Void);
  get_current_token(lexer);
  return ast_node;
}

// jumps are goto identifier; continue; break; return;
ASTNode *parse_jump_statement(Lexer *lexer) {
  ASTNode *ast_node = new_ast_node(ASTNodeType::Void);
  get_current_token(lexer);
  return ast_node;
}

void parse_file(const char *file) {
  Lexer lexer = new_lexer(file);
  Scope current_scope;

  for (const Token *current_token = get_next_token(&lexer);
       current_token->type != TokenType::Eof;
       current_token = get_next_token(&lexer)) {

    if (token_is_declaration_specifier(current_token, &current_scope)) {
      parse_declaration(&lexer, &current_scope);
    }
  }
}
