#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

// from crafting interpreters, ch 16
static char *read_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (file == NULL) {
    fprintf(stderr, "Could not open file %s, aborting.\n", path);
  }

  fseek(file, 0L, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Could not buffer file %s, aborting.\n", path);
  }

  long bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file %s, aborting.\n", path);
  }

  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}

int main(int argc, char **argv) {

  for (int i = 1; i < argc; i++) {
    if (access(argv[i], F_OK) == 0) {
      char *buffer = read_file(argv[i]);
      (void)buffer;
    } else {
      fprintf(stderr, "File %s not found, aborting.\n", argv[i]);
    }
  }

  return 0;
}
