#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sstd/sstd.h"

#define ASCII_DEL EXPAND(127)
#define CARET_OFFSET EXPAND(64)

#define OPT_NONE EXPAND(0)

// -b --number-nonblank (overrides -n --number)
#define OPT_NUMBER_NONBLANK MKFLAG(0)

// -n --number
#define OPT_NUMBER MKFLAG(1)

// -s --squeeze-blank
#define OPT_SQUEEZE_BLANK MKFLAG(2)

// -v --show-nonprinting: use ^ and M- notation, except for LFD and TAB
#define OPT_SHOW_NONPRINTING MKFLAG(3)

// -T --show-tabs  (display tabs as ^I)
#define OPT_SHOW_TABS MKFLAG(4)

// -E --show-ends (display end of each line with $)
#define OPT_SHOW_ENDS MKFLAG(5)

// -A --show-all  (eq. to -vET)
#define OPT_SHOW_ALL \
  EXPAND(OPT_SHOW_NONPRINTING | OPT_SHOW_ENDS | OPT_SHOW_TABS)

// NOTE:
//  * -e (-vE)
//  * -t (-vT)
//  * -u (ignored)

#define OPT_HELP MKFLAG(10)

#define MAKE_LONG_OPT(NAME, OPT) \
  { (NAME), no_argument, NULL, (OPT) }

static const struct option LONG_OPTIONS[] = {
    MAKE_LONG_OPT("help", OPT_HELP),
    MAKE_LONG_OPT("number-nonblank", OPT_NUMBER_NONBLANK),
    MAKE_LONG_OPT("number", OPT_NUMBER),
    MAKE_LONG_OPT("squeeze-blank", OPT_SQUEEZE_BLANK),
    MAKE_LONG_OPT("show-nonprinting", OPT_SHOW_NONPRINTING),
    MAKE_LONG_OPT("show-tabs", OPT_SHOW_TABS),
    MAKE_LONG_OPT("show-ends", OPT_SHOW_ENDS),
    MAKE_LONG_OPT("show-all", OPT_SHOW_ALL),
};

static const char *SHORT_OPTS = "bnsvTEAetu";

static bool is_caret_printable(char c) {
  return (c < ' ' || c == ASCII_DEL) && c != '\t' && c != '\n';
}

static bool is_m_printable(unsigned char c) { return c > CHAR_MAX; }

/*
 * :returns: Returns nonprintable char in caret notation
 * */
static char get_caret_notation(char c) {
  char ret = 0;

  if (c < ' ' && c != '\t' && c != '\n') {
    ret = c + CARET_OFFSET;
  } else if (c == ASCII_DEL) {
    ret = c - CARET_OFFSET;
  }

  return ret;
}

static char get_m_caret_notation(unsigned char c) {
  char ret = c - CHAR_MAX - 1;
  if (ret < ' ') {
    ret += CARET_OFFSET;
  }
  return ret;
}

static void fprint_prefix(FILE *out, unsigned long long *linenumber,
                          bool isnewline, bool isblank, int opts) {
  // Kinda hacky but anyway
  if (HASFLAG(opts, OPT_NUMBER_NONBLANK) && isnewline && !isblank) {
    fprintf(out, "%6llu\t", *linenumber);
    ++(*linenumber);
  } else if (HASFLAG(opts, OPT_NUMBER) && isnewline &&
             !(HASFLAG(opts, OPT_NUMBER_NONBLANK))) {
    fprintf(out, "%6llu\t", *linenumber);
    ++(*linenumber);
  }
}

static void fputchar_opts(FILE *out, char curchar, int opts) {
  if (HASFLAG(opts, OPT_SHOW_ENDS) && curchar == '\n') {
    fputs("$\n", out);
  } else if (HASFLAG(opts, OPT_SHOW_TABS) && curchar == '\t') {
    fputs("^I", out);
  } else if (HASFLAG(opts, OPT_SHOW_NONPRINTING) &&
             is_m_printable((unsigned char)curchar)) {
    if ((unsigned char)curchar >= CHAR_MAX + 1 &&
        ((unsigned char)curchar - CHAR_MAX - 1) < ' ') {
      fprintf(out, "M-^%c", get_m_caret_notation(curchar));
    } else {
      fprintf(out, "M-%c", get_m_caret_notation(curchar));
    }
  } else if (HASFLAG(opts, OPT_SHOW_NONPRINTING) &&
             is_caret_printable(curchar)) {
    fprintf(out, "^%c", get_caret_notation(curchar));
  } else {
    fputc(curchar, out);
  }
}

static long long fprint_fd_opts(FILE *in, FILE *out, int opts) {
  if (in == NULL || out == NULL) {
    return -1;
  }

  unsigned long long charcount = 0, linenumber = 1, blankcount = 0;
  char curchar = 0, prevchar = 0;

  while ((curchar = fgetc(in)) != EOF) {
    bool isblank = curchar == '\n' && (prevchar == '\n' || charcount == 0);
    bool isnewline = prevchar == '\n' || charcount == 0;

    if (isblank) {
      ++blankcount;
    } else {
      blankcount = 0;
    }

    if (HASFLAG(opts, OPT_SQUEEZE_BLANK) && isblank && blankcount > 1) {
      // Skip it
    } else {
      fprint_prefix(out, &linenumber, isnewline, isblank, opts);
      fputchar_opts(out, curchar, opts);
      prevchar = curchar;
    }

    ++charcount;
  }

  return charcount;
}

static void print_help(void) {
  printf(
      "USAGE\n"
      "    s21_cat [OPTIONS...] [FILES...]\n"
      "\n"
      "OPTIONS\n"
      "    -b --number-nonblank  (numbers only non-empty lines)\n"
      "    -n --number           (number all output lines)\n"
      "    -s --squeeze-blank    (squeeze multiple adjacent blank lines)\n"
      "    -v --show-nonprinting (use ^ and M- notation, except for LF and "
      "TAB)\n"
      "    -T --show-tabs        (display tabs as ^I)\n"
      "    -E --show-ends        (display end of each line with $)\n"
      "    -A --show-all         (same as -vET)\n"
      "    -e                    (same as -vE)\n"
      "    -t                    (same as -vT)\n"
      "    -h --help             (display this help message)\n");
}

static int make_opts(int *out_mask, int argc, char *const *argv) {
  int ret = EXIT_SUCCESS, opt_value = 0, opt_index = 0;

  while ((opt_value = getopt_long(argc, argv, SHORT_OPTS, LONG_OPTIONS,
                                  &opt_index)) != -1) {
    if (opt_value == OPT_HELP || opt_value == 'h') {
      ADDFLAG(*out_mask, OPT_HELP);
    } else if (opt_value == OPT_NUMBER || opt_value == 'n') {
      // -n --number
      ADDFLAG(*out_mask, OPT_NUMBER);
    } else if (opt_value == OPT_NUMBER_NONBLANK || opt_value == 'b') {
      // -b --number-nonblank (overrides -n --number)
      ADDFLAG(*out_mask, OPT_NUMBER_NONBLANK);
    } else if (opt_value == OPT_SQUEEZE_BLANK || opt_value == 's') {
      // -s --squeeze-blank
      ADDFLAG(*out_mask, OPT_SQUEEZE_BLANK);
    } else if (opt_value == OPT_SHOW_NONPRINTING || opt_value == 'v') {
      // -v --show-nonprinting: use ^ and M- notation, except for LFD and TAB
      ADDFLAG(*out_mask, OPT_SHOW_NONPRINTING);
    } else if (opt_value == OPT_SHOW_TABS || opt_value == 'T') {
      // -T --show-tabs  (display tabs as ^I)
      ADDFLAG(*out_mask, OPT_SHOW_TABS);
    } else if (opt_value == OPT_SHOW_ENDS || opt_value == 'E') {
      // -E --show-ends (display end of each line with $)
      ADDFLAG(*out_mask, OPT_SHOW_ENDS);
    } else if (opt_value == OPT_SHOW_ALL || opt_value == 'A') {
      // -A --show-all  (eq. to -vET)
      ADDFLAG(*out_mask, OPT_SHOW_ALL);
    } else if (opt_value == 'e') {
      ADDFLAG(*out_mask, OPT_SHOW_NONPRINTING | OPT_SHOW_ENDS);
    } else if (opt_value == 't') {
      ADDFLAG(*out_mask, OPT_SHOW_NONPRINTING | OPT_SHOW_TABS);
    } else if (opt_value == 'u') {
      // Ignore
    } else {
      ret = EXIT_FAILURE;
    }
  }

  return ret;
}

static int process_files(const char **f_paths, size_t count, int opts) {
  int ret = EXIT_SUCCESS;

  for (size_t i = 0; i < count; ++i) {
    const char *f_path = f_paths[i];
    FILE *f_in = fopen(f_path, "r");

    if (fprint_fd_opts(f_in, stdout, opts) == -1) {
      fprintf(stderr, "error: Failed to find file '%s'\n", f_path);
      ret = EXIT_FAILURE;
    }

    if (f_in != NULL) {
      fclose(f_in);
    }
  }

  return ret;
}

int main(int argc, char **argv) {
  int ret = EXIT_SUCCESS, opts = OPT_NONE;

  if (make_opts(&opts, argc, argv) != EXIT_SUCCESS) {
    ret = EXIT_FAILURE;
  } else {
    if (HASFLAG(opts, OPT_HELP)) {
      print_help();
    } else {
      size_t args_left = argc - optind;
      const char **f_paths = (const char **)(argv + optind);
      process_files(f_paths, args_left, opts);
    }
  }

  return ret;
}
