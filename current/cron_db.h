#ifndef EWB_CRON_DB_H
#define EWB_CRON_DB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ewb_cron_row
{ char *task;
  char *cronstring;
} ewb_cron_row;

typedef struct ewb_cron_job
{ char *task;
  char *program_url;
  char *stack;
  char *parameters;
  char *cronstring;
} ewb_cron_job;

int ewb_cron_list(ewb_cron_row **rows_out, int *count_out);
void ewb_cron_list_free(ewb_cron_row *rows, int count);
int ewb_cron_load_job(const char *task, ewb_cron_job *job);
void ewb_cron_job_free(ewb_cron_job *job);

#ifdef __cplusplus
}
#endif

#endif
