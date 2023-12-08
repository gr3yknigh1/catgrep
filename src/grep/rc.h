#ifndef GREP_RC_H_
#define GREP_RC_H_

/*
 * Return Code
 * */
typedef enum {
  RC_END = -1,
  RC_OK = 0,
  RC_PATTERN_NOT_FOUND = 1,
  RC_ERROR = 2,
  RC_FILE_NOT_FOUND = 3,
} rc_t;

#endif  // GREP_RC_H_
