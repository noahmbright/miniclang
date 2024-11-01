#include "lexer.h"

#include <cassert>
#include <cstdio>

bool token_equals(Token const* left, Token const* right)
{
  return left->type == right->type && left->string == right->string;
}

static void lexer_update_start_of_token(Lexer* lexer)
{
  lexer->beginning_of_current_token = lexer->current_location;
  lexer->beginning_of_token_column = lexer->current_column;
  lexer->beginning_of_token_line = lexer->current_line;
}

static void advance(Lexer* lexer)
{
  if (*lexer->current_location == '\0')
    return;
  lexer->current_location++;
  lexer->current_column++;
}

Lexer new_lexer(char const* text)
{
  Lexer lexer;
  // printf("initializing lexer with input: %s\n", text);

  lexer.current_filepath = text;
  lexer.current_location = text;
  lexer.beginning_of_current_token = text;

  lexer.beginning_of_token_line = 0;
  lexer.beginning_of_token_column = 0;

  lexer.current_line = 0;
  lexer.current_column = 0;

  lexer.current_token.type = TokenType::NotStarted;

  return lexer;
}

Token* get_current_token(Lexer* lexer) { return &lexer->current_token; }

Token recover_and_return_error_token(Lexer* lexer, Token error_token)
{
  Token const* current_token = get_next_token(lexer);
  while (current_token->type != TokenType::Semicolon && current_token->type != TokenType::Eof)
    current_token = get_next_token(lexer);

  return error_token;
}

void lexer_print_error_message(Lexer* lexer, char const* message)
{
  fprintf(stderr, "Error: %s Line %d:%d  :\n", lexer->current_filepath,
      lexer->beginning_of_token_line, lexer->beginning_of_token_column);

  fprintf(stderr, "%*s^\n", lexer->beginning_of_token_column + 7, "");

  fprintf(stderr, "%s\n", message);
}

bool expect_token_type(Token const* token, TokenType type)
{
  return token->type == type;
}

Token const* expect_next_token_and_skip(Lexer* lexer, TokenType type,
    char const* error_message)
{

  if (expect_token_type(get_next_token(lexer), type))
    return get_next_token(lexer);

  lexer->current_token = error_token(lexer, error_message);
  return &lexer->current_token;
}

Token const* expect_and_get_next_token(Lexer* lexer, TokenType type,
    char const* error_message)
{

  if (expect_token_type(get_current_token(lexer), type))
    return get_next_token(lexer);

  lexer->current_token = error_token(lexer, error_message);
  return &lexer->current_token;
}

static Token lexer_make_token_without_advancing(Lexer* lexer,
    TokenType token_type,
    std::string string = "")
{
  return make_token(token_type, lexer->beginning_of_token_line,
      lexer->beginning_of_token_column, string);
}

static Token lexer_make_token_and_advance(Lexer* lexer, TokenType token_type,
    std::string string = "")
{

  Token token = make_token(token_type, lexer->beginning_of_token_line,
      lexer->beginning_of_token_column, string);
  advance(lexer);
  return token;
}

Token make_token(TokenType token_type, unsigned line, unsigned column,
    std::string string)
{
  Token token;

  token.type = token_type;
  token.line = line;
  token.column = column;
  token.string = string;

  return token;
}

Token error_token(Lexer* lexer, char const* error_message)
{
  lexer_print_error_message(lexer, error_message);
  exit(1);
  return lexer_make_token_and_advance(lexer, TokenType::Error);
}

static bool is_non_digit(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool is_alphanumeric(char c) { return is_digit(c) || is_non_digit(c); }

static bool is_whitespace(char c)
{
  return c == '\r' || c == ' ' || c == '\t' || c == '\n';
}

static void new_line(Lexer* lexer)
{
  lexer->current_line++;
  lexer->current_column = 0;
}

static char current_char(Lexer* lexer) { return *lexer->current_location; }

static char char_lookahead(Lexer* lexer, int n)
{
  for (int i = 0; i < n; i++)
    if (lexer->current_location[i] == '\0')
      return '\0';
  return lexer->current_location[n];
}

static char peek_next_char(Lexer* lexer) { return char_lookahead(lexer, 1); }

// static char prev_char(Lexer *lexer) { return char_lookahead(lexer, -1); }

static void skip_whitespace(Lexer* lexer)
{
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

static void skip_line_comment(Lexer* lexer)
{
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

static void skip_block_comment(Lexer* lexer)
{
  assert(current_char(lexer) == '/' && peek_next_char(lexer) == '*');
  while (current_char(lexer) != '\0' || !(current_char(lexer) == '*' && peek_next_char(lexer) == '/')) {

    if (current_char(lexer) == '\n')
      new_line(lexer);

    advance(lexer);
  }
}

bool is_on_line_comment(Lexer* lexer)
{

  return (current_char(lexer) == '/' && peek_next_char(lexer) == '/');
}

bool is_on_block_comment(Lexer* lexer)
{
  return (current_char(lexer) == '/' && peek_next_char(lexer) == '*');
}

static void skip_whitespace_and_comments(Lexer* lexer)
{
  while (is_whitespace(current_char(lexer)) || is_on_block_comment(lexer) || is_on_line_comment(lexer)) {
    if (is_whitespace(current_char(lexer)))
      skip_whitespace(lexer);

    if (is_on_line_comment(lexer))
      skip_line_comment(lexer);

    if (is_on_block_comment(lexer))
      skip_block_comment(lexer);
  }
}

// this works if current_location is the position after the last char of the
// token
static unsigned token_length(Lexer* lexer)
{
  return lexer->current_location - lexer->beginning_of_current_token;
}

static std::string string_from_lexer(Lexer* lexer)
{
  return std::string(lexer->beginning_of_current_token,
      lexer->beginning_of_current_token + token_length(lexer));
}

/*
static Token lex_string_literal(Lexer *lexer) {
  assert(current_char(lexer) == '"');
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
*/

bool token_is_integer_suffix(Token* token)
{
  TokenType type = token->type;
  switch (type) {
  case TokenType::IntegerSuffixl:
  case TokenType::IntegerSuffixL:
  case TokenType::IntegerSuffixu:
  case TokenType::IntegerSuffixU:
  case TokenType::IntegerSuffixll:
  case TokenType::IntegerSuffixLL:
  case TokenType::IntegerSuffixull:
  case TokenType::IntegerSuffixuLL:
  case TokenType::IntegerSuffixllu:
  case TokenType::IntegerSuffixLLu:
  case TokenType::IntegerSuffixUll:
  case TokenType::IntegerSuffixULL:
  case TokenType::IntegerSuffixllU:
  case TokenType::IntegerSuffixLLU:
    return true;
  default:
    return false;
  }
}

/*static Token lex_integer_suffix(Lexer *lexer) {
  while (is_alphanumeric(current_char(lexer)))
    advance(lexer);

  std::string suffix = string_from_lexer(lexer);

  if (suffix == "l")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixl);
  if (suffix == "L")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixL);
  if (suffix == "u")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixu);
  if (suffix == "U")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixU);
  if (suffix == "ll")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixll);
  if (suffix == "LL")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixLL);
  if (suffix == "ull")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixull);
  if (suffix == "llu")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixllu);
  if (suffix == "LLu")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixLLu);
  if (suffix == "Ull")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixUll);
  if (suffix == "ULL")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixULL);
  if (suffix == "llU")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixllU);
  if (suffix == "LLU")
    return lexer_make_token_and_advance(lexer, TokenType::IntegerSuffixLLU);

  return error_token(lexer, "invalid integer suffix");
}
*/

static bool is_hex_digit(char c)
{
  bool is_upper_hex = (c >= 'A' && c <= 'F');
  bool is_lower_hex = (c >= 'a' && c <= 'f');
  return is_digit(c) || is_lower_hex || is_upper_hex;
}

static bool is_octal_digit(char c) { return c <= '7' && c >= '0'; }

static bool is_binary_digit(char c) { return c == '0' || c == '1'; }

Token lex_hexadecimal_number(Lexer* lexer)
{

  assert(current_char(lexer) == '0' && "lex_hexadecimal_number not starting on 0");
  advance(lexer);

  assert(current_char(lexer) == 'x' && "lex_hexadecimal_number second char not x");
  advance(lexer);

  if (!is_hex_digit(current_char(lexer)))
    return error_token(
        lexer,
        "numeric constant prefixed with 0x, but no hex digits following");

  while (is_hex_digit(current_char(lexer)))
    advance(lexer);

  return lexer_make_token_without_advancing(lexer, TokenType::Number);
}

Token lex_binary_number(Lexer* lexer)
{

  assert(current_char(lexer) == '0' && "lex_binary_number not starting on 0");
  advance(lexer);

  assert(current_char(lexer) == 'b' && "lex_binary_number second char not b");
  advance(lexer);

  if (!is_binary_digit(current_char(lexer)))
    return error_token(
        lexer,
        "numeric constant prefixed with 0b, but no binary digits following");

  while (is_binary_digit(current_char(lexer)))
    advance(lexer);

  return lexer_make_token_without_advancing(lexer, TokenType::Number);
}

Token lex_octal_number(Lexer* lexer)
{

  assert(current_char(lexer) == '0' && "lex_octal_number not starting on 0");
  advance(lexer);

  if (!is_octal_digit(current_char(lexer)))
    return error_token(
        lexer,
        "numeric constant prefixed with 0, but no octal digits following");

  while (is_octal_digit(current_char(lexer)))
    advance(lexer);

  return lexer_make_token_without_advancing(lexer, TokenType::Number);
}

// need to deal with signed/unsigned hex, decimal, octal, binary integers
// need to deal with floats/double
Token lex_number(Lexer* lexer)
{
  lexer_update_start_of_token(lexer);

  bool seen_decimal_point = false;
  char c = current_char(lexer);

  // hex, octal, binary
  // FIXME: Also floats starting with 0. ?
  if (c == '0') {
    char next_char = peek_next_char(lexer);

    if (next_char == 'x')
      return lex_hexadecimal_number(lexer);

    if (next_char == 'b')
      return lex_binary_number(lexer);

    if (is_digit(next_char))
      return lex_octal_number(lexer);
  }

  // default lexing of decimal constant, potentially integer or floating point
  while (is_digit(c) /*|| c == '.' || is_non_digit(c)*/) {

    if (c == '.') {
      if (seen_decimal_point)
        return error_token(lexer, "Found second decimal point in number");

      seen_decimal_point = true;
    }

    if (is_non_digit(c)) {
      switch (c) {
      case 'u':
      case 'U':
      }
    }

    advance(lexer);
    c = current_char(lexer);
  }

  return lexer_make_token_without_advancing(lexer, TokenType::Number,
      string_from_lexer(lexer));
}

static char peek_char_in_token(Lexer* lexer, unsigned idx)
{
  unsigned int length = token_length(lexer);
  if (idx < length)
    return lexer->beginning_of_current_token[idx];
  return '\0';
}

static Token lex_ellipses(Lexer* lexer)
{
  assert(current_char(lexer) == '.');
  advance(lexer);
  assert(current_char(lexer) == '.');
  advance(lexer);
  assert(current_char(lexer) == '.');
  advance(lexer);
  return lexer_make_token_and_advance(lexer, TokenType::Ellipsis);
}

static Token token_from_keyword_or_identifier(Lexer* lexer,
    TokenType token_type,
    std::string keyword)
{
  std::string current_lexer_string = string_from_lexer(lexer);

  if (current_lexer_string == keyword)
    return lexer_make_token_without_advancing(lexer, token_type);

  return lexer_make_token_without_advancing(lexer, TokenType::Identifier,
      current_lexer_string);
}

Token lex_next_token(Lexer* lexer)
{
  skip_whitespace_and_comments(lexer);
  lexer_update_start_of_token(lexer);

  if (is_digit(current_char(lexer)) || (current_char(lexer) == '.' && is_digit(peek_next_char(lexer))))
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

  case '^':
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::XorEquals);
    }
    return lexer_make_token_and_advance(lexer, TokenType::Caret);

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
    // positive float
    if (is_digit(peek_next_char(lexer)) || peek_next_char(lexer) == '.')
      return lex_number(lexer);

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

    // negative float
    if (is_digit(peek_next_char(lexer)) || peek_next_char(lexer) == '.')
      return lex_number(lexer);

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

  case '%':
    if (peek_next_char(lexer) == '=') {
      advance(lexer);
      return lexer_make_token_and_advance(lexer, TokenType::ModuloEquals);
    }
    return lexer_make_token_and_advance(lexer, TokenType::Modulo);

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

  switch (peek_char_in_token(lexer, 0)) {
  case '_':
    switch (peek_char_in_token(lexer, 1)) {
    case 'A':
      switch (peek_char_in_token(lexer, 2)) {
      case 't':
        return token_from_keyword_or_identifier(lexer, TokenType::Atomic,
            "_Atomic");
      case 'l':
        return token_from_keyword_or_identifier(lexer, TokenType::AlignAs,
            "_Alignas");
      }
    case 'N':
      return token_from_keyword_or_identifier(lexer, TokenType::NoReturn,
          "_Noreturn");
    case 'T':
      return token_from_keyword_or_identifier(lexer, TokenType::ThreadLocal,
          "_Thread_local");
    }

  case 'a':
    return token_from_keyword_or_identifier(lexer, TokenType::Auto, "auto");
  case 'b':
    return token_from_keyword_or_identifier(lexer, TokenType::Break, "break");

  case 'c':
    switch (peek_char_in_token(lexer, 1)) {
    case 'a':
      return token_from_keyword_or_identifier(lexer, TokenType::Case, "case");
    case 'h':
      return token_from_keyword_or_identifier(lexer, TokenType::Char, "char");

    case 'o':
      // const and continue both have [2] = n
      switch (peek_char_in_token(lexer, 3)) {
      case 's':
        return token_from_keyword_or_identifier(lexer, TokenType::Const,
            "const");
      case 't':
        return token_from_keyword_or_identifier(lexer, TokenType::Continue,
            "continue");
      }
    }

  case 'd':
    switch (peek_char_in_token(lexer, 1)) {
    case 'e':
      return token_from_keyword_or_identifier(lexer, TokenType::Default,
          "default");
    case 'o':
      if (peek_char_in_token(lexer, 2) == 'u')
        return token_from_keyword_or_identifier(lexer, TokenType::Double,
            "double");
      return token_from_keyword_or_identifier(lexer, TokenType::Do, "do");
    }

  case 'e':
    switch (peek_char_in_token(lexer, 1)) {
    case 'l':
      return token_from_keyword_or_identifier(lexer, TokenType::Else, "else");
    case 'n':
      return token_from_keyword_or_identifier(lexer, TokenType::Enum, "enum");
    case 'x':
      return token_from_keyword_or_identifier(lexer, TokenType::Extern,
          "extern");
    }

  case 'f':
    switch (peek_char_in_token(lexer, 1)) {
    case 'l':
      return token_from_keyword_or_identifier(lexer, TokenType::Float, "float");
    case 'o':
      return token_from_keyword_or_identifier(lexer, TokenType::For, "for");
    }

  case 'g':
    return token_from_keyword_or_identifier(lexer, TokenType::GoTo, "goto");

  case 'i':
    switch (peek_char_in_token(lexer, 1)) {
    case 'n':
      switch (peek_char_in_token(lexer, 2)) {
      case 't':
        return token_from_keyword_or_identifier(lexer, TokenType::Int, "int");
      case 'l':
        return token_from_keyword_or_identifier(lexer, TokenType::Inline,
            "inline");
      }
    case 'f':
      return token_from_keyword_or_identifier(lexer, TokenType::If, "if");
    }

  case 'l':
    return token_from_keyword_or_identifier(lexer, TokenType::Long, "long");

    // register, return, restrict all start with "re"
  case 'r':
    switch (peek_char_in_token(lexer, 2)) {
    case 'g':
      return token_from_keyword_or_identifier(lexer, TokenType::Register,
          "register");
    case 't':
      return token_from_keyword_or_identifier(lexer, TokenType::Return,
          "return");
    case 's':
      return token_from_keyword_or_identifier(lexer, TokenType::Restrict,
          "restrict");
    }

  case 's':
    switch (peek_char_in_token(lexer, 1)) {
    case 'h':
      return token_from_keyword_or_identifier(lexer, TokenType::Short, "short");
    case 'i':
      switch (peek_char_in_token(lexer, 2)) {
      case 'g':
        return token_from_keyword_or_identifier(lexer, TokenType::Signed,
            "signed");
      case 'z':
        return token_from_keyword_or_identifier(lexer, TokenType::SizeOf,
            "sizeof");
      }
    case 't':
      switch (peek_char_in_token(lexer, 2)) {
      case 'r':
        return token_from_keyword_or_identifier(lexer, TokenType::Struct,
            "struct");
      case 'a':
        return token_from_keyword_or_identifier(lexer, TokenType::Static,
            "static");
      }
    case 'w':
      return token_from_keyword_or_identifier(lexer, TokenType::Switch,
          "switch");
    }

  case 't':
    return token_from_keyword_or_identifier(lexer, TokenType::Typedef,
        "typedef");

    // union, unsigned
  case 'u':
    switch (peek_char_in_token(lexer, 2)) {
    case 'i':
      return token_from_keyword_or_identifier(lexer, TokenType::Union, "union");
    case 's':
      return token_from_keyword_or_identifier(lexer, TokenType::Unsigned,
          "unsigned");
    }

    // void, volatile
  case 'v':
    switch (peek_char_in_token(lexer, 2)) {
    case 'i':
      return token_from_keyword_or_identifier(lexer, TokenType::Void, "void");
    case 'l':
      return token_from_keyword_or_identifier(lexer, TokenType::Volatile,
          "volatile");
    }

  case 'w':
    return token_from_keyword_or_identifier(lexer, TokenType::While, "while");

  default:
    return lexer_make_token_without_advancing(lexer, TokenType::Identifier,
        string_from_lexer(lexer));
  }

  assert(false && "Lex next token UNREACHABLE");
}

Token const* get_next_token(Lexer* lexer)
{
  if (lexer->current_token.type == TokenType::Eof)
    return &lexer->current_token;

  lexer->current_token = lex_next_token(lexer);
  return &lexer->current_token;
}
