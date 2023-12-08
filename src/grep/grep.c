#define _GNU_SOURCE
#include <getopt.h>
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: Replace with appropriate macro from std
#define UNIX_FD_STDIN 0
#define UNIX_FD_STDOUT 1
#define UNIX_FD_STDERR 2

// In FreeBSD implementation this symbol isn't defined
#ifndef REG_NOERROR
#define REG_NOERROR 0

// SILLY LIBC IMPL DETECTION
#define SILLY_MUSL_IMPL 1

#endif  // REG_NOERROR

#ifndef SSTD_MEMORY_IMPL
#define SSTD_MEMORY_IMPL
#endif  // SSTD_MEMORY_IMPL

#include "patterns.h"
#include "rc.h"
#include "sstd/memory.h"
#include "sstd/bits.h"
#include "sstd/color.h"

#define OPT_NONE EXPAND(0)
#define OPT_HELP MKFLAG(1)
#define OPT_REGEXP MKFLAG(2)
#define OPT_FILE MKFLAG(3)
#define OPT_IGNORE_CASE MKFLAG(4)
#define OPT_INVERT_MATCH MKFLAG(5)
#define OPT_COUNT MKFLAG(6)
#define OPT_FILES_WITH_MATCHES MKFLAG(7)
#define OPT_ONLY_MATCHING MKFLAG(8)
#define OPT_NO_MESSAGES MKFLAG(9)
#define OPT_LINE_NUMBER MKFLAG(10)
#define OPT_NO_FILENAME MKFLAG(11)
#define OPT_NO_COLOR MKFLAG(12)

#define MAKE_FLAG_OPT(__NAME, __OPT) \
  { (__NAME), no_argument, NULL, (__OPT) }

#define MAKE_PARAM_OPT(__NAME, __OPT) \
  { (__NAME), required_argument, NULL, (__OPT) }

#define MATCH_COLOR ANSI_BOLD_RED
#define FILENAME_COLOR ANSI_BOLD_PURPLE
#define LINENUM_COLOR ANSI_GREEN
#define LINESEP_COLOR ANSI_CYAN

#ifdef CONFIG_DEBUG
#define MAX_MATCHES 32
#else
#define MAX_MATCHES 2048  // 256
#endif                    // CONFIG_DEBUG

typedef unsigned int optmask_t;
typedef int regopt_t;

static void print_matches(optmask_t optmask, const char *line,
                          regmatch_t *matches, size_t match_count,
                          size_t line_number);

static void print_short_usage(void);
static void print_help(void);

static char *alloc_str_from_buf(const char *s, size_t size);

static regopt_t make_from_optmask(optmask_t optmask);

static rc_t alloc_regexes_and_compile(regex_t **regexes,
                                      const patterns_t *patterns,
                                      optmask_t optmask);

static size_t search_line_for_matches(regmatch_t *matches,
                                      const regex_t *regexes,
                                      const patterns_t *patterns,
                                      const char *line_buffer,
                                      size_t line_buffer_size);

static rc_t search_file_for_matches(const patterns_t *patterns,
                                    optmask_t optmask, FILE *file,
                                    const char *file_path);
static rc_t process_argsleft(patterns_t *patterns, optmask_t optmask,
                             int argsleft, FILE **file, char **argv,
                             const char **file_path);
static rc_t gather_optmask_and_patterns(optmask_t *optmask,
                                        patterns_t *patterns, int argc,
                                        char **argv, int *argsleft);

static void print_line_count_if_should(optmask_t optmask, size_t line_matched,
                                       size_t lines_total,
                                       const char *const file_path);

static void print_filename_with_matches_if_should(optmask_t optmask,
                                                  const char *const file_path,
                                                  size_t line_matched);

static void fclose_if_not_null(FILE *file);

int main(int argc, char **argv) {
  rc_t rc = RC_OK;
  optmask_t optmask = OPT_NONE;
  patterns_t patterns = patterns_init();

  const char *file_path = NULL;
  FILE *file = NULL;
  int argsleft = 0;

  if (!isatty(UNIX_FD_STDOUT)) {
    ADDFLAG(optmask, OPT_NO_COLOR);
  }

  if (gather_optmask_and_patterns(&optmask, &patterns, argc, argv, &argsleft) ==
      RC_OK) {
    if (HASFLAG(optmask, OPT_HELP)) {
      print_help();
    } else if (process_argsleft(&patterns, optmask, argsleft, &file, argv,
                                &file_path) == RC_OK) {
      if (argc - optind == 1) {
        ADDFLAG(optmask, OPT_NO_FILENAME);
      }

      for (; optind < argc; ++optind) {
        file_path = argv[optind];
        file = fopen(file_path, "r");

        if (file == NULL) {
          fprintf(stderr, "error: %s: No such file or directory\n", file_path);
          rc = RC_ERROR;
        } else {
          rc = search_file_for_matches(&patterns, optmask, file, file_path);
        }
        fclose_if_not_null(file);
      }
    } else {
      rc = RC_ERROR;
    }
  } else {
    rc = RC_ERROR;
  }

  // fclose_if_not_null(file);
  patterns_free(&patterns);

  return rc;
}

static char *alloc_str_from_buf(const char *buf, size_t size) {
  char *str = calloc(size + 1, sizeof(char));
  memory_copy(str, buf, size);
  str[size] = '\0';
  return str;
}

static regopt_t make_from_optmask(optmask_t optmask) {
  regopt_t regopt = REG_NEWLINE | REG_EXTENDED;

  if (HASFLAG(optmask, OPT_IGNORE_CASE)) {
    ADDFLAG(regopt, REG_ICASE);
  }

  return regopt;
}

static rc_t alloc_regexes_and_compile(regex_t **regexes,
                                      const patterns_t *patterns,
                                      optmask_t optmask) {
  rc_t rc = RC_OK;

  if (!(patterns->count > 0)) {
    rc = RC_PATTERN_NOT_FOUND;
  }

  if (rc == RC_OK) {
    *regexes = calloc(patterns->count, sizeof(regex_t));
  }

  for (size_t i = 0; rc == RC_OK && i < patterns->count; ++i) {
    if (regcomp((*regexes) + i, patterns->data[i],
                make_from_optmask(optmask)) != RC_OK) {
      rc = RC_ERROR;
    }
  }

  return rc;
}

static void print_filename_prefix_if_should(optmask_t optmask,
                                            const char *file_path) {
  if (!HASFLAG(optmask, OPT_NO_FILENAME)) {
    if (HASFLAG(optmask, OPT_NO_COLOR)) {
      printf("%s:", file_path);
    } else {
      USE_FG(FILENAME_COLOR) { printf("%s", file_path); }
      {
        USE_FG(LINESEP_COLOR) { putchar(':'); }
      }
    }
  }
}

static void print_matches_if_should(optmask_t optmask, const char *line,
                                    regmatch_t *matches, size_t match_count,
                                    size_t line_number, size_t *line_matched,
                                    const char *file_path) {
  bool hasmatches = match_count > 0;
  bool should_print_this_line =
      ((hasmatches && !HASFLAG(optmask, OPT_INVERT_MATCH)) ||
       (!hasmatches && HASFLAG(optmask, OPT_INVERT_MATCH)));

  bool should_print = !HASFLAG(optmask, OPT_FILES_WITH_MATCHES) &&
                      !HASFLAG(optmask, OPT_COUNT) && should_print_this_line;

  if (should_print) {
    print_filename_prefix_if_should(optmask, file_path);
    print_matches(optmask, line, matches, match_count, line_number);
  }

  if (hasmatches) {
    ++(*line_matched);
  }
}

static void print_line_count_if_should(optmask_t optmask, size_t line_matched,
                                       size_t lines_total,
                                       const char *const file_path) {
  size_t count = HASFLAG(optmask, OPT_INVERT_MATCH) ? lines_total - line_matched
                                                    : line_matched;

  if (HASFLAG(optmask, OPT_COUNT)) {
    print_filename_prefix_if_should(optmask, file_path);
    printf("%zu\n", count);
  }
}

static void print_filename_with_matches_if_should(optmask_t optmask,
                                                  const char *const file_path,
                                                  size_t line_matched) {
  if (HASFLAG(optmask, OPT_FILES_WITH_MATCHES) && line_matched > 0) {
    if (HASFLAG(optmask, OPT_NO_COLOR)) {
      printf("%s\n", file_path);
    } else {
      USE_FG(FILENAME_COLOR) { printf("%s\n", file_path); }
    }
  }
}

static size_t search_line_for_matches(regmatch_t *matches,
                                      const regex_t *regexes,
                                      const patterns_t *patterns,
                                      const char *line_buffer,
                                      size_t line_buffer_size) {
  size_t match_count = 0;
  // NOTE(wittenbb): `getline` returns non-nullterminated string, but
  // `regexec` requires one
  char *search_str = alloc_str_from_buf(line_buffer, line_buffer_size);
  char *search_ptr = search_str;

  for (size_t pattern_idx = 0;
       match_count < MAX_MATCHES && pattern_idx < patterns->count;
       ++pattern_idx) {
    int regexec_rc = RC_OK;
    size_t search_off = 0;
    while (match_count < MAX_MATCHES &&
           (regexec_rc = regexec(regexes + pattern_idx, search_ptr + search_off,
                                 MAX_MATCHES - match_count,
                                 matches + match_count, 0)) == REG_NOERROR &&
           regexec_rc != REG_NOMATCH) {
      regmatch_t *match = matches + match_count;
      match->rm_so += search_off;
      match->rm_eo += search_off;
      search_off = matches[match_count].rm_eo;
      match_count++;
    }
  }
  free(search_str);
  return match_count;
}

static rc_t search_file_for_matches(const patterns_t *patterns,
                                    optmask_t optmask, FILE *file,
                                    const char *file_path) {
  rc_t rc = RC_OK;
  regex_t *regexes = NULL;
  char *line_buffer = NULL;
  size_t line_buffer_size = 0, line_matched = 0, lines_count = 0;

  rc = alloc_regexes_and_compile(&regexes, patterns, optmask);

  while (rc == RC_OK &&
         getline(&line_buffer, &line_buffer_size, file) != RC_END) {
    ++lines_count;

    regmatch_t matches[MAX_MATCHES] = {0};
    size_t match_count = search_line_for_matches(matches, regexes, patterns,
                                                 line_buffer, line_buffer_size);
    print_matches_if_should(optmask, line_buffer, matches, match_count,
                            lines_count, &line_matched, file_path);
  }

  // #ifndef SILLY_MUSL_IMPL
  // print_filename_with_matches_if_should(optmask, file_path, line_matched);
  // print_line_count_if_should(optmask, line_matched, lines_count, file_path);
  // #else
  if (rc != RC_PATTERN_NOT_FOUND) {
    print_filename_with_matches_if_should(optmask, file_path, line_matched);
    print_line_count_if_should(optmask, line_matched, lines_count, file_path);
  }
  // #endif

  for (size_t i = 0; i < patterns->count; ++i) {
    regfree(regexes + i);
  }
  free_if_not_null(line_buffer);
  free_if_not_null(regexes);

  if (line_matched == 0) {
    rc = RC_PATTERN_NOT_FOUND;
  }

  return rc;
}

static rc_t process_argsleft(patterns_t *patterns, optmask_t optmask,
                             int argsleft, FILE **file, char **argv,
                             const char **file_path) {
  rc_t rc = RC_OK;
  if (argsleft > 0) {
    if (argsleft > 1 && !HASFLAG(optmask, OPT_REGEXP) &&
        !HASFLAG(optmask, OPT_FILE)) {
      patterns_push(patterns, argv[optind++]);
    }
    // TODO: Remove it
    KEEP(file);
    KEEP(file_path);
  } else {
    print_short_usage();
    rc = RC_ERROR;
  }
  return rc;
}

static rc_t gather_optmask_and_patterns(optmask_t *optmask,
                                        patterns_t *patterns, int argc,
                                        char **argv, int *argsleft) {
  static const char *SHORT_OPTS = "e:f:icvlosnh";
  static const struct option LONG_OPTS[] = {
      MAKE_FLAG_OPT("regexp", OPT_REGEXP),
      MAKE_FLAG_OPT("file", OPT_FILE),
      MAKE_FLAG_OPT("ignore-case", OPT_IGNORE_CASE),
      MAKE_FLAG_OPT("invert-match", OPT_INVERT_MATCH),
      MAKE_FLAG_OPT("count", OPT_COUNT),
      MAKE_FLAG_OPT("files-with-matches", OPT_FILES_WITH_MATCHES),
      MAKE_FLAG_OPT("only-matching", OPT_ONLY_MATCHING),
      MAKE_FLAG_OPT("no-messages", OPT_NO_MESSAGES),
      MAKE_FLAG_OPT("line-number", OPT_LINE_NUMBER),
      MAKE_FLAG_OPT("no-filename", OPT_NO_FILENAME),
      MAKE_FLAG_OPT("help", OPT_HELP),
  };

  rc_t rc = RC_OK;
  int opt_value = 0, opt_index = 0;

  while (rc == RC_OK &&
         (opt_value = getopt_long(argc, argv, SHORT_OPTS, LONG_OPTS,
                                  &opt_index)) != RC_END) {
    switch (opt_value) {
      case OPT_HELP:
        ADDFLAG(*optmask, OPT_HELP);
        break;

      case 'e':
      case OPT_REGEXP:
        ADDFLAG(*optmask, OPT_REGEXP);
        patterns_push(patterns, optarg);
        break;

      case 'f':
      case OPT_FILE:
        ADDFLAG(*optmask, OPT_FILE);
        if (patterns_push_file(patterns, optarg) == RC_FILE_NOT_FOUND) {
          fprintf(stderr, "error: %s: No such file or directory\n", optarg);
          rc = RC_ERROR;
        }
        break;

      case 'i':
      case OPT_IGNORE_CASE:
        ADDFLAG(*optmask, OPT_IGNORE_CASE);
        break;

      case 'v':
      case OPT_INVERT_MATCH:
        ADDFLAG(*optmask, OPT_INVERT_MATCH);
        break;

      case 'c':
      case OPT_COUNT:
        ADDFLAG(*optmask, OPT_COUNT);
        break;

      case 'l':
      case OPT_FILES_WITH_MATCHES:
        ADDFLAG(*optmask, OPT_FILES_WITH_MATCHES);
        break;

      case 'n':
      case OPT_LINE_NUMBER:
        ADDFLAG(*optmask, OPT_LINE_NUMBER);
        break;
    }
  }

  *argsleft = argc - optind;

  return rc;
}

static void print_matches(optmask_t optmask, const char *line,
                          regmatch_t *matches, size_t match_count,
                          size_t line_number) {
  const char *line_ptr = line;
  size_t line_idx = 0;

  if (HASFLAG(optmask, OPT_LINE_NUMBER)) {
    // TODO: Replace with new SSTD_COLOR API
    if (HASFLAG(optmask, OPT_NO_COLOR)) {
      printf("%zu:", line_number);
    } else {
      USE_FG(LINENUM_COLOR) { printf("%zu", line_number); }
      {
        USE_FG(LINESEP_COLOR) { putchar(':'); }
      }
    }
  }

  while (*line_ptr != '\0') {
    bool isinmatch = false;
    for (size_t match_idx = 0; match_idx < match_count; ++match_idx) {
      const regmatch_t *match = matches + match_idx;
      ssize_t match_off = match->rm_so;
      ssize_t match_end = match->rm_eo;

      if (ISINRANGE((size_t)match_off, line_idx, (size_t)match_end)) {
        isinmatch = true;
      }
    }

    if (isinmatch) {
      if (HASFLAG(optmask, OPT_NO_COLOR)) {
        putchar(*line_ptr);
      } else {
        USE_FG(MATCH_COLOR) { putchar(*line_ptr); }
      }
    } else {
      putchar(*line_ptr);
    }

    ++line_ptr;
    ++line_idx;
  }
}

static void print_short_usage(void) {
  printf(
      "USAGE\n"
      "\n"
      "    s21_grep [OPTIONS...] [PATTERN] [FILE]\n"
      "\n"
      "Try 's21_grep --help' for more informations.\n");
}

static void print_help(void) {
  printf(
      "USAGE\n"
      "\n"
      "    s21_grep [OPTIONS...] [PATTERN] [FILE]\n"
      "\n"
      "OPTIONS\n"
      "\n"
      "    Matching Control\n"
      "    -e PATTERN --regexp PATTERN (search pattern)\n"
      "    -f FILE    --file   FILE    (obtain patterns from FILE, one per "
      "line)\n"
      "    -i         --ignore-case    (ignore case distinctions in patterns)\n"
      "    -v         --invert-match   (invert the snse of matching, to select "
      "non-matching lines)\n"
      "\n"
      "    General Output Control\n"
      "    -c --count              (suppress normal output; print a count of "
      "matching lines)\n"
      "    -l --files-with-matches (suppress normal output; print the name of "
      "each input file with maching patterns)\n"
      "    -o --only-matching      (print only the matched [aka non-empty] "
      "parts of matching line, with each such part on a separte output line) "
      "[NOIMPL]\n"
      "    -s --no-messages        (suppress error messages about nonexistent "
      "or unreadable files) [NOIMPL]\n"
      "\n"
      "    Output Prefix control\n"
      "    -n --line-number  (prefix each line of output with the 1-based line "
      "number within its input file)\n"
      "    -h, --no-filename (suppress the prefixing of file names on output; "
      "this is the default when there is only one file to search) [PARTIMPL]\n"
      "\n");
}

static void fclose_if_not_null(FILE *file) {
  if (file != NULL) {
    fclose(file);
  }
}
