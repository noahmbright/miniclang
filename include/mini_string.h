#pragma once

struct String {
  const char *pointer;
  unsigned length;
};

inline bool string_equals(String left, String right) {
  return left.pointer == right.pointer && left.length == right.length;
}
