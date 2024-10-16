#include "lexer.h"

#include <cassert>
#include <cstdio>

bool token_equals(Token *left, Token *right) {
  return left->type == right->type && left->string == right->string;
}

static void lexer_update_start_of_token(Lexer *lexer) {
  lexer->beginning_of_current_token = lexer->current_location;
  lexer->beginning_of_token_column = lexer->current_column;
  lexer->beginning_of_token_line = lexer->current_line;
}

static void advance(Lexer *lexer) {
  lexer->current_location++;
  lexer->current_column++;
}

Lexer new_lexer(const char *text) {
  Lexer lexer;

  lexer.current_filepath = text;
  lexer.current_location = text;
  lexer.beginning_of_current_token = text;

  lexer.beginning_of_token_line = 0;
  lexer.beginning_of_token_column = 0;

  lexer.current_line = 0;
  lexer.current_column = 0;

  return lexer;
}

Token *get_current_token(Lexer *lexer) { return &lexer->current_token; }

Token recover_and_return_error_token(Lexer *lexer, Token error_token) {
  Token *current_token = get_next_token(lexer);
  while (current_token->type != TokenType::Semicolon &&
         current_token->type != TokenType::Eof)
    current_token = get_next_token(lexer);

  return error_token;
}

void lexer_print_error_message(Lexer *lexer, const char *message) {
  fprintf(stderr, "Error: %s Line %d:%d  :\n %s", lexer->current_filepath,
          lexer->beginning_of_token_line, lexer->beginning_of_token_column,
          message);
}

bool expect_token_type(Token *token, TokenType type) {
  return token->type == type;
}

Token *expect_and_get_next_token(Lexer *lexer, TokenType type,
                                 const char *error_message) {

  Token *next_tok = get_next_token(lexer);
  if (expect_token_type(next_tok, type))
    return next_tok;

  lexer->current_token = error_token(lexer, error_message);
  return &lexer->current_token;
}

static Token lexer_make_token_without_advancing(Lexer *lexer,
                                                TokenType token_type,
                                                std::string string = "") {
  return make_token(token_type, lexer->beginning_of_token_line,
                    lexer->beginning_of_token_column, string);
}

static Token lexer_make_token_and_advance(Lexer *lexer, TokenType token_type,
                                          std::string string = "") {

  Token token = make_token(token_type, lexer->beginning_of_token_line,
                           lexer->beginning_of_token_column, string);
  advance(lexer);
  return token;
}

Token make_token(TokenType token_type, unsigned line, unsigned column,
                 std::string string) {
  Token token;

  token.type = token_type;
  token.line = line;
  token.column = column;
  token.string = string;

  return token;
}

Token error_token(Lexer *lexer, const char *error_message) {
  lexer_print_error_message(lexer, error_message);
  return lexer_make_token_and_advance(lexer, TokenType::Error);
}

static bool is_non_digit(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool is_alphanumeric(char c) { return is_digit(c) || is_non_digit(c); }

static bool is_whitespace(char c) {
  return c == '\r' || c == ' ' || c == '\t' || c == '\n';
}

static void new_line(Lexer *lexer) {
  lexer->current_line++;
  lexer->current_column = 0;
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

static void skip_whitespace(Lexer *lexer) {
  while (is_whitespace(current_char(lexer))) {
    switch (current_char(lexer)) {
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

static void skip_line_comment(Lexer *lexer) {
  // presumes lexer starts on first / out of the //
  char c = current_char(lexer);
  assert(c == '/' && peek_next_char(lexer) == '/');

  while (c != '\n' && c != '\0')
    advance(lexer);

  if (c == '\n') {
    new_line(lexer);
    advance(lexer);
  }
}

static void skip_block_comment(Lexer *lexer) {
  assert(current_char(lexer) == '/' && peek_next_char(lexer) == '*');
  while (current_char(lexer) != '\0' ||
         !(current_char(lexer) == '*' && peek_next_char(lexer) == '/')) {

    if (current_char(lexer) == '\n')
      new_line(lexer);

    advance(lexer);
  }
}

bool is_on_line_comment(Lexer *lexer) {

  return (current_char(lexer) == '/' && peek_next_char(lexer) == '/');
}

bool is_on_block_comment(Lexer *lexer) {
  return (current_char(lexer) == '/' && peek_next_char(lexer) == '*');
}

static void skip_whitespace_and_comments(Lexer *lexer) {
  while (is_whitespace(current_char(lexer)) || is_on_block_comment(lexer) ||
         is_on_line_comment(lexer)) {
    if (is_whitespace(current_char(lexer)))
      skip_whitespace(lexer);

    if (is_on_line_comment(lexer))
      skip_line_comment(lexer);

    if (is_on_block_comment(lexer))
      skip_block_comment(lexer);
  }
}

static unsigned token_length(Lexer *lexer) {
  return lexer->current_location - lexer->beginning_of_current_token;
}

static std::string string_from_lexer(Lexer *lexer) {
  return std::string(lexer->beginning_of_current_token,
                     lexer->beginning_of_current_token + token_length(lexer));
}

static Token lex_string_literal(Lexer *lexer) {
  assert(current_char(lexer) == '"');
  advance(lexer);
  lexer->beginning_of_current_token = lexer->current_location;

  while (peek_next_char(lexer) != '"' && current_char(lexer) != '\\')
    advance(lexer);

  Token token = lexer_make_token_and_advance(lexer, TokenType::StringLiteral,
                                             string_from_lexer(lexer));

  advance(lexer);
  assert(current_char(lexer) == '"');
  advance(lexer);

  return token;
}

Token lex_number(Lexer *lexer) {
  assert(current_char(lexer));
  while (is_digit(current_char(lexer)))
    advance(lexer);

  return lexer_make_token_without_advancing(lexer, TokenType::Number,
                                            string_from_lexer(lexer));
}

Token lex_ellipses(Lexer *lexer) {
  assert(current_char(lexer) == '.');
  advance(lexer);
  assert(current_char(lexer) == '.');
  advance(lexer);
  assert(current_char(lexer) == '.');
  advance(lexer);
  return lexer_make_token_and_advance(lexer, TokenType::Ellipses);
}

static Token token_from_keyword_or_identifier(Lexer *lexer,
                                              TokenType token_type,
                                              std::string keyword) {
  std::string current_lexer_string = string_from_lexer(lexer);
  if (current_lexer_string == keyword)
    return lexer_make_token_without_advancing(lexer, token_type);

  return lexer_make_token_without_advancing(lexer, TokenType::Identifier,
                                            current_lexer_string);
}

Token lex_next_token(Lexer *lexer) {
  skip_whitespace_and_comments(lexer);
  lexer_update_start_of_token(lexer);

  if (is_digit(current_char(lexer)) ||
      (current_char(lexer) == '.' && is_digit(peek_next_char(lexer))))
    return lex_number(lexer);

  // punctuation
  switch (current_char(lexer)) {
  case '\0':
    return lexer_make_token_and_advance(lexer, TokenType::Eof);
  case ',':
    return lexer_make_token_and_advance(lexer, TokenType::Comma);

  case '{':
    return lexer_make_token_and_advance(lexer, TokenType::LBrace);
  case '}':
    return lexer_make_token_and_advance(lexer, TokenType::RBrace);
  case '(':
    return lexer_make_token_and_advance(lexer, TokenType::LParen);
  case ')':
    return lexer_make_token_and_advance(lexer, TokenType::RParen);
  case '[':
    return lexer_make_token_and_advance(lexer, TokenType::LBracket);
  case ']':
    return lexer_make_token_and_advance(lexer, TokenType::RBracket);
  case ';':
    return lexer_make_token_and_advance(lexer, TokenType::Semicolon);
  case ':':
    return lexer_make_token_and_advance(lexer, TokenType::Colon);
  case '?':
    return lexer_make_token_and_advance(lexer, TokenType::QuestionMark);

  case '.':
    if (peek_next_char(lexer) == '.')
      return lex_ellipses(lexer);

    return lexer_make_token_and_advance(lexer, TokenType::Dot);

  case '!':
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::NotEquals);
    }
    return lexer_make_token_and_advance(lexer, TokenType::Bang);

  case '+':
    // +=
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::PlusEquals);
    }
    // ++
    if (peek_next_char(lexer) == '+') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::PlusPlus);
    }

    // +
    return lexer_make_token_and_advance(lexer, TokenType::Plus);

  case '-':
    // -=
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::MinusEquals);
    }

    // --
    if (peek_next_char(lexer) == '-') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::MinusMinus);
    }

    // -
    return lexer_make_token_and_advance(lexer, TokenType::Minus);

  case '*':
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::TimesEquals);
    }
    return lexer_make_token_and_advance(lexer, TokenType::Asterisk);

  case '/':
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::DividedByEquals);
    }
    return lexer_make_token_and_advance(lexer, TokenType::ForwardSlash);

  case '<':
    // <=
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::LessThanOrEqualTo);
    }

    if (peek_next_char(lexer) == '<') {
      advance(lexer);
      // <<=
      if (peek_next_char(lexer) == '=') {
        advance(lexer);
        return lexer_make_token_and_advance(lexer,
                                            TokenType::BitShiftLeftEquals);
      }

      // <<
      return lexer_make_token_and_advance(lexer, TokenType::BitShiftLeft);
    }

    return lexer_make_token_and_advance(lexer, TokenType::LessThan);

  case '>':
    // >=
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer,
                                          TokenType::GreaterThanOrEqualTo);
    }

    // >>
    if (peek_next_char(lexer) == '>') {
      advance(lexer);
      // >>=
      if (peek_next_char(lexer) == '=') {
        advance(lexer);
        return lexer_make_token_and_advance(lexer,
                                            TokenType::BitShiftRightEquals);
      }

      return lexer_make_token_and_advance(lexer, TokenType::BitShiftRight);
    }
    return lexer_make_token_and_advance(lexer, TokenType::GreaterThan);

  case '=':
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::DoubleEquals);
    }
    return lexer_make_token_and_advance(lexer, TokenType::Equals);

  case '&':
    if (peek_next_char(lexer) == '&') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::LogicalAnd);
    }
    return lexer_make_token_and_advance(lexer, TokenType::Ampersand);

  case '|':
    if (peek_next_char(lexer) == '|') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::LogicalOr);
    }
    return lexer_make_token_and_advance(lexer, TokenType::Pipe);
  }

  // alphabetical - have lexer run until next nonalphanumeric
  // and decide what keyword it's found
  // if not a keyword, then an identifier
  while (is_alphanumeric(current_char(lexer)))
    advance(lexer);

  switch (lexer->beginning_of_current_token[0]) {
  case 'i':
    switch (lexer->beginning_of_current_token[1]) {
    case 'n':
      return token_from_keyword_or_identifier(lexer, TokenType::Int, "int");
    case 'f':
    }
  default:
    return lexer_make_token_without_advancing(lexer, TokenType::Identifier,
                                              string_from_lexer(lexer));
  }

  return {};
}

Token *get_next_token(Lexer *lexer) {
  lexer->current_token = lex_next_token(lexer);
  return &lexer->current_token;
}
