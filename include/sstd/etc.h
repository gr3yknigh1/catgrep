#ifndef SSTD_ETC_H_
#define SSTD_ETC_H_

#define KEEP(__X) ((void)(__X))
#define EXPAND(__X) (__X)
#define STRINGIFY(__X) #__X

#define CTX(__BEFORE, __AFTER) \
  char __ctx_do_out = 1;       \
  for (KEEP(__BEFORE); __ctx_do_out--; KEEP(__AFTER))

#define ISINRANGE(__MIN, __X, __MAX) ((__MIN) <= (__X) && (__X) < (__MAX))

#endif  // SSTD_ETC_H_
