#ifndef SSTD_BITS_H_
#define SSTD_BITS_H_

#include <stdio.h>

#include "etc.h"

#define FPUTBITS(__X, __FD)                                           \
  do {                                                                \
    for (unsigned long __index = sizeof(__X) * CHAR_BIT; __index > 0; \
         --__index) {                                                 \
      putc((__X) & (1 << (__index - 1)) ? '1' : '0', __FD);           \
    }                                                                 \
  } while (0)

#define PUTBITS(__X) FPUTBITS((__X), stdout)
#define PRINTBITS(__X)       \
  do {                       \
    FPUTBITS((__X), stdout); \
    putchar('\n');           \
  } while (0)

#define MKFLAG(__X) EXPAND(1 << (__X))

#define HASFLAG(__MASK, __X) EXPAND((__MASK) & (__X))
#define ADDFLAG(__MASK, __X) EXPAND((__MASK) |= (__X))
#define RMFLAG(__MASK, __X) ((__MASK) ^= (__X))

#endif  // SSTD_BITS_H_
