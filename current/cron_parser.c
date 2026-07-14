#include "cron_parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct cron_name
{ const char *name;
  int value;
} cron_name;

static const cron_name month_names[] =
{ {"jan",1},
  {"feb",2},
  {"mar",3},
  {"apr",4},
  {"may",5},
  {"jun",6},
  {"jul",7},
  {"aug",8},
  {"sep",9},
  {"oct",10},
  {"nov",11},
  {"dec",12},
  {NULL,0}
};

static const cron_name dow_names[] =
{ {"sun",7},
  {"mon",1},
  {"tue",2},
  {"wed",3},
  {"thu",4},
  {"fri",5},
  {"sat",6},
  {NULL,0}
};

static void set_error(char *err, size_t errsz, const char *msg)
{
  if (err == NULL || errsz == 0) return;
  snprintf(err, errsz, "%s", msg);
}

static int is_number_string(const char *s)
{
  size_t i;

  if (s == NULL || s[0] == 0) return 0;
  for (i = 0; s[i] != 0; i++)
  { if (!isdigit((unsigned char)s[i])) return 0;
  }
  return 1;
}

static void lower_copy(char *dst, size_t dstsz, const char *src)
{
  size_t i;

  if (dstsz == 0) return;
  for (i = 0; src[i] != 0 && i + 1 < dstsz; i++)
    dst[i] = (char)tolower((unsigned char)src[i]);
  dst[i] = 0;
}

static int lookup_name(const cron_name *names, const char *token, int *value)
{
  char low[32];
  int i;

  lower_copy(low, sizeof(low), token);
  for (i = 0; names[i].name != NULL; i++)
  { if (!strcmp(low, names[i].name))
    { *value = names[i].value;
      return 1;
    }
  }
  return 0;
}

static int parse_field_value(const char *token, int field_id, int *value)
{
  long v;

  if (field_id == 3)
  { if (lookup_name(month_names, token, value)) return 1;
  }
  else if (field_id == 4)
  { if (lookup_name(dow_names, token, value)) return 1;
  }

  if (!is_number_string(token)) return 0;
  v = strtol(token, NULL, 10);
  *value = (int)v;
  return 1;
}

static int field_min(int field_id)
{
  if (field_id == 0) return 0;
  if (field_id == 1) return 0;
  if (field_id == 2) return 1;
  if (field_id == 3) return 1;
  return 0;
}

static int field_max(int field_id)
{
  if (field_id == 0) return 59;
  if (field_id == 1) return 23;
  if (field_id == 2) return 31;
  if (field_id == 3) return 12;
  return 7;
}

static int value_in_range(int field_id, int value)
{
  if (value < field_min(field_id)) return 0;
  if (value > field_max(field_id)) return 0;
  return 1;
}

static int mask_contains(uint64_t mask, int value)
{
  if (value < 0 || value > 63) return 0;
  return (mask & (1ULL << value)) != 0;
}

static void set_mask_bit(uint64_t *mask, int field_id, int raw_value)
{
  int value = raw_value;

  if (field_id == 4 && value == 7) value = 0;
  *mask |= (1ULL << value);
}

static int apply_range(uint64_t *mask, int field_id, int start, int end, int step, char *err, size_t errsz)
{
  int i;

  if (step <= 0)
  { set_error(err, errsz, "Cron step must be positive");
    return 0;
  }
  if (!value_in_range(field_id, start) || !value_in_range(field_id, end))
  { set_error(err, errsz, "Cron field out of range");
    return 0;
  }
  if (start > end)
  { set_error(err, errsz, "Cron range reversed");
    return 0;
  }

  for (i = start; i <= end; i += step)
    set_mask_bit(mask, field_id, i);
  return 1;
}

static int split_once(const char *src, char sep, char *left, size_t lsz, char *right, size_t rsz)
{
  const char *p;
  size_t nleft;

  p = strchr(src, sep);
  if (p == NULL) return 0;
  nleft = (size_t)(p - src);
  if (nleft + 1 > lsz) return 0;
  if (strlen(p + 1) + 1 > rsz) return 0;
  memcpy(left, src, nleft);
  left[nleft] = 0;
  strcpy(right, p + 1);
  return 1;
}

static int parse_item(const char *item, int field_id, uint64_t *mask, char *err, size_t errsz)
{
  char base[64];
  char stepbuf[32];
  char left[64];
  char right[64];
  int step;
  int start;
  int end;

  if (strchr(item, '/'))
  { if (!split_once(item, '/', base, sizeof(base), stepbuf, sizeof(stepbuf)))
    { set_error(err, errsz, "Cron step syntax");
      return 0;
    }
    if (!is_number_string(stepbuf))
    { set_error(err, errsz, "Cron step syntax");
      return 0;
    }
    step = atoi(stepbuf);
  }
  else
  { strcpy(base, item);
    step = 1;
  }

  if (!strcmp(base, "*"))
    return apply_range(mask, field_id, field_min(field_id), field_max(field_id), step, err, errsz);

  if (strchr(base, '-'))
  { if (!split_once(base, '-', left, sizeof(left), right, sizeof(right)))
    { set_error(err, errsz, "Cron range syntax");
      return 0;
    }
    if (!parse_field_value(left, field_id, &start) || !parse_field_value(right, field_id, &end))
    { set_error(err, errsz, "Cron field value");
      return 0;
    }
    return apply_range(mask, field_id, start, end, step, err, errsz);
  }

  if (!parse_field_value(base, field_id, &start))
  { set_error(err, errsz, "Cron field value");
    return 0;
  }
  return apply_range(mask, field_id, start, start, step, err, errsz);
}

static int parse_field(const char *text, int field_id, uint64_t *mask, int *is_any, char *err, size_t errsz)
{
  const char *p;
  char item[64];
  size_t n;

  *mask = 0;
  *is_any = 0;

  if (!strcmp(text, "*"))
  { *is_any = 1;
  }

  p = text;
  while (*p != 0)
  { const char *comma = strchr(p, ',');
    if (comma == NULL) n = strlen(p);
    else n = (size_t)(comma - p);
    if (n == 0 || n >= sizeof(item))
    { set_error(err, errsz, "Cron list syntax");
      return 0;
    }
    memcpy(item, p, n);
    item[n] = 0;
    if (!parse_item(item, field_id, mask, err, errsz)) return 0;
    if (comma == NULL) break;
    p = comma + 1;
  }

  return 1;
}

int cron_parse(const char *expr, ewb_cron *out, char *err, size_t errsz)
{
  char copy[256];
  char *fields[5];
  char *tok;
  int count;
  int any;

  if (expr == NULL || out == NULL)
  { set_error(err, errsz, "Cron parser arguments");
    return 0;
  }
  if (strlen(expr) >= sizeof(copy))
  { set_error(err, errsz, "Cron expression too long");
    return 0;
  }

  strcpy(copy, expr);
  count = 0;
  tok = strtok(copy, " \t\r\n");
  while (tok != NULL && count < 5)
  { fields[count] = tok;
    count++;
    tok = strtok(NULL, " \t\r\n");
  }

  if (tok != NULL || count != 5)
  { set_error(err, errsz, "Cron needs 5 fields");
    return 0;
  }

  if (!parse_field(fields[0], 0, &out->minute_mask, &any, err, errsz)) return 0;
  if (!parse_field(fields[1], 1, &out->hour_mask, &any, err, errsz)) return 0;
  if (!parse_field(fields[2], 2, &out->dom_mask, &out->dom_any, err, errsz)) return 0;
  if (!parse_field(fields[3], 3, &out->month_mask, &any, err, errsz)) return 0;
  if (!parse_field(fields[4], 4, &out->dow_mask, &out->dow_any, err, errsz)) return 0;

  return 1;
}

int cron_match(const ewb_cron *cron, const struct tm *tmv)
{
  int dom_ok;
  int dow_ok;

  if (cron == NULL || tmv == NULL) return 0;

  if (!mask_contains(cron->minute_mask, tmv->tm_min)) return 0;
  if (!mask_contains(cron->hour_mask, tmv->tm_hour)) return 0;
  if (!mask_contains(cron->month_mask, tmv->tm_mon + 1)) return 0;

  dom_ok = mask_contains(cron->dom_mask, tmv->tm_mday);
  dow_ok = mask_contains(cron->dow_mask, tmv->tm_wday);

  if (cron->dom_any && cron->dow_any) return 1;
  if (cron->dom_any) return dow_ok;
  if (cron->dow_any) return dom_ok;
  if (dom_ok) return 1;
  if (dow_ok) return 1;
  return 0;
}
