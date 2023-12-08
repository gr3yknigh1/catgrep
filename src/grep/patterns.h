#ifndef GREP_PATTERNS_H_
#define GREP_PATTERNS_H_

#include <stdlib.h>

#include "rc.h"

typedef struct {
  char **data;
  size_t count;
  size_t capacity;
} patterns_t;

patterns_t patterns_init(void);
void patterns_push(patterns_t *patterns, const char *pattern);
void patterns_free(patterns_t *patterns);
rc_t patterns_push_file(patterns_t *patterns, const char *file_path);

#endif  // GREP_PATTERNS_H_
