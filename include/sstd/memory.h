/*
 * SMOLL MEMORY LIB
 *
 * NOTICE: This is single-header lib, so yep, we got here definition and
 * implementation at the same time
 * */
#ifndef SSTD_MEMORY_H_
#define SSTD_MEMORY_H_

#include <stddef.h>

void *memory_copy(void *dst, const void *restrict src, size_t size);
void free_if_not_null(void *p);

#ifdef SSTD_MEMORY_IMPL

void *memory_copy(void *dst, const void *restrict src, size_t size) {
  do {
    ((char *)dst)[size - 1] = ((char *)src)[size - 1];
    size--;
  } while (size > 0);
  return dst;
}

void free_if_not_null(void *p) {
  if (p != NULL) {
    free(p);
  }
}

#endif  // SSTD_MEMORY_IMPL

#endif  // SSTD_MEMORY_H_
