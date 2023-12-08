/*
 * SMOLL STRING LIB
 *
 * NOTICE: This is single-header lib, so yep, we got here definition and
 * implementation at the same time
 * */
#ifndef SSTD_STR_H_
#define SSTD_STR_H_

#include <stdlib.h>

size_t str_copy(char *dst, const char *restrict src);
void str_replace_char(char *s, char to_replace, char replace_with);

#ifdef SSTD_STR_IMPL

size_t str_copy(char *dst, const char *restrict src) {
  size_t ccount = 0;

  while (src[ccount] != '\0') {
    dst[ccount] = src[ccount];
    ++ccount;
  }

  return ccount;
}

void str_replace_char(char *s, char to_replace, char replace_with) {
  do {
    if (*s == to_replace) {
      *s = replace_with;
    }
  } while (*(++s) != '\0');
}

#endif  // SSTD_STR_IMPL

#endif  // SSTD_STR_H_
