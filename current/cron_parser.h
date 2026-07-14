#ifndef EWB_CRON_PARSER_H
#define EWB_CRON_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ewb_cron
{ uint64_t minute_mask;
  uint64_t hour_mask;
  uint64_t dom_mask;
  uint64_t month_mask;
  uint64_t dow_mask;
  int dom_any;
  int dow_any;
} ewb_cron;

int cron_parse(const char *expr, ewb_cron *out, char *err, size_t errsz);
int cron_match(const ewb_cron *cron, const struct tm *tmv);

#ifdef __cplusplus
}
#endif

#endif
