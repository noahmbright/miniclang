#include "lexer.h"

#include <cassert>
#include <cstdio>

Lexer new_lexer(const char *text) {
  Lexer lexer;
  lexer.current_filepath = text;
  lexer.current_location = text;
  lexer.beginning_of_current_token = text;
  lexer.line = 0;
  lexer.column = 0;
  return lexer;
}

static Token lex_next_token(Lexer *lexer) {}

Token *get_next_token(Lexer *lexer) {
  lexer->current_token = lex_next_token(lexer);
  return &lexer->current_token;
}

Token recover_and_return_error_token(Lexer *lexer, Token error_token) {
  Token *current_token = get_next_token(lexer);
  while (current_token->type != TokenType::Semicolon &&
         current_token->type != TokenType::Eof)
    current_token = get_next_token(lexer);

  return error_token;
}

Token error_token(Lexer *lexer, const char *error_message) {
  lexer_print_error_message(lexer, error_message);
  return make_token(lexer, TokenType::Error);
}

void lexer_print_error_message(Lexer *lexer, const char *message) {
  fprintf(stderr, "Error: %s Line %d:%d  :\n %s", lexer->current_filepath,
          lexer->line, lexer->column, message);
}

bool expect_token_type(Token token, TokenType type) {
  return token.type == type;
}

Token *expect_and_skip(Lexer *lexer, TokenType type,
                       const char *error_message) {

  Token *next_tok = get_next_token(lexer);
  if (expect_token_type(next_tok, type))
    return next_tok;

  lexer->current_token = error_token(lexer, error_message);
  return &lexer->current_token;
}

Token make_token(Lexer *lexer, TokenType token_type) {
  Token token;

  token.type = token_type;
  token.string.length =
      lexer->current_location - lexer->beginning_of_current_token;
  token.string.pointer = lexer->beginning_of_current_token;
  token.line = lexer->line;
  token.column = lexer->column;

  return token;
}

static bool is_non_digit(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool is_whitespace(char c) {
  return c == '\r' || c == ' ' || c == '\t' || c == '\n';
}

static void advance(Lexer *lexer) {
  lexer->current_location++;
  lexer->column++;
}

static void new_line(Lexer *lexer) {
  lexer->line++;
  lexer->column = 0;
}

static void skip_whitespace(Lexer *lexer) {
  while (is_whitespace(*lexer->current_location)) {
    switch (*lexer->current_location) {
    case '\n':
      new_line(lexer);
    case ' ':
    case '\t':
    case '\r':
      advance(lexer);
      break;
    }
  }
}

static char current_char(Lexer *lexer) { return *lexer->current_location; }

static char char_lookahead(Lexer *lexer, int n) {
  for (int i = 0; i < n; i++)
    if (lexer->current_location[i] == '\0')
      return '\0';
  return lexer->current_location[n];
}

static char peek_next_char(Lexer *lexer) { return char_lookahead(lexer, 1); }

static char prev_char(Lexer *lexer) { return char_lookahead(lexer, -1); }

static unsigned token_length(Lexer *lexer) {
  return lexer->current_location - lexer->beginning_of_current_token;
}

static void skip_line_comment(Lexer *lexer) {
  // presumes lexer starts on first / our of the //
  char c = current_char(lexer);
  assert(c == '/' && c == '/');

  while (c != '\n' && c != '\0')
    advance(lexer);

  assert(current_char(lexer) == '\n');
  new_line(lexer);
  advance(lexer);
}

static String lex_string_literal(Lexer *lexer) {
  // TODO: Decide if string should start on quote or not
  // Starting on quote for now for symmetry with scanning

  assert(current_char(lexer) == '"');
  lexer->beginning_of_current_token = lexer->current_location;

  String string;
  string.pointer = lexer->beginning_of_current_token;
  while (current_char(lexer) != '"' && prev_char(lexer) != '\\')
    advance(lexer);

  string.length = token_length(lexer);
  return string;
}

static Token check_keyword() {}

TokenType lex_type(Lexer *lexer) {
  skip_whitespace(lexer);

  lexer->beginning_of_current_token = lexer->current_location;

  switch (*lexer->current_location) {
  case '/':
    if (peek_next_char(lexer) == '/')
      skip_line_comment(lexer);
  case 'i':
    switch (char_lookahead(lexer, 1)) {
    case 'n':
    case 'f':
    }
  }

  return {};
}
