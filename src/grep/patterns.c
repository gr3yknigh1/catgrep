#include "patterns.h"

#define _GNU_SOURCE
#include <regex.h>
#include <stdio.h>
#include <string.h>

#ifndef SSTD_STR_IMPL
#define SSTD_STR_IMPL
#endif  // SSTD_STR_IMPL

#include "sstd/memory.h"
#include "sstd/str.h"

patterns_t patterns_init(void) {
  return (patterns_t){
      .data = NULL,
      .count = 0,
      .capacity = 0,
  };
}

void patterns_push(patterns_t *patterns, const char *pattern) {
  if (patterns->capacity == 0) {
    patterns->data = malloc(sizeof(char *));
    patterns->capacity++;
  } else if (patterns->count + 1 >= patterns->capacity) {
    patterns->capacity *= 2;
    patterns->data =
        realloc(patterns->data, patterns->capacity * sizeof(char *));
  }

  size_t pattern_len = strlen(pattern);
  patterns->data[patterns->count] = malloc(pattern_len + 1);
  str_copy(patterns->data[patterns->count], pattern);
  patterns->data[patterns->count][pattern_len] = '\0';

  patterns->count++;
}

void patterns_free(patterns_t *patterns) {
  for (size_t i = 0; i < patterns->count; ++i) {
    free(patterns->data[i]);
  }
  free_if_not_null(patterns->data);
}

rc_t patterns_push_file(patterns_t *patterns, const char *file_path) {
  rc_t rc = RC_OK;

  FILE *file = fopen(file_path, "r");

  if (file == NULL) {
    rc = RC_FILE_NOT_FOUND;
  } else {
    char *line = NULL;
    size_t line_size = 0;

    while (getline(&line, &line_size, file) != RC_END) {
      str_replace_char(line, '\n', '\0');
      patterns_push(patterns, line);
    }

    free_if_not_null(line);
    fclose(file);
  }

  return rc;
}
