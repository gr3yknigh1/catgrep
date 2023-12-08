#include <limits.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
  int ret = 0;

  FILE *file = fopen("gen_file.txt", "w");

  for (uint8_t c = 0; c < UCHAR_MAX; ++c) {
    fprintf(file, "[ %3i ]  %c", (unsigned char)c, c);
    fputc('\n', file);
  }

  fclose(file);

  return ret;
}
