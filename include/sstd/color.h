#ifndef SSTD_COLOR_H_
#define SSTD_COLOR_H_

#include <stdio.h>

#include "etc.h"

// TODO: Add generic ANSI macros for BOLD and ITALIC and BG colors

#define ANSI_BLACK "\033[0;30m"
#define ANSI_RED "\033[0;31m"
#define ANSI_GREEN "\033[0;32m"
#define ANSI_YELLOW "\033[0;33m"
#define ANSI_BLUE "\033[0;34m"
#define ANSI_PURPLE "\033[0;35m"
#define ANSI_CYAN "\033[0;36m"
#define ANSI_WHITE "\033[0;37m"
// #define ANSI_RESET "\033[0m"
#define ANSI_RESET "\33[m\33[K"

#define ANSI_BOLD_BLACK "\033[1;30m"
#define ANSI_BOLD_RED "\033[1;31m"
#define ANSI_BOLD_GREEN "\033[1;32m"
#define ANSI_BOLD_YELLOW "\033[1;33m"
#define ANSI_BOLD_BLUE "\033[1;34m"
#define ANSI_BOLD_PURPLE "\033[1;35m"
#define ANSI_BOLD_CYAN "\033[1;36m"
#define ANSI_BOLD_WHITE "\033[1;37m"

#define F_USE_FG(__FD, __ANSI_COLOR) \
  CTX(fputs(__ANSI_COLOR, __FD), fputs(ANSI_RESET, __FD))

#define USE_FG(__ANSI_COLOR) F_USE_FG(stdout, __ANSI_COLOR)

#endif  // SSTD_COLOR_H_
