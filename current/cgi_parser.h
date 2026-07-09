#ifndef EWB_CGI_PARSER_H
#define EWB_CGI_PARSER_H

#include <stddef.h>

#define EWB_MAX_FIELDS 512
#define EWB_MAX_FILES 128

typedef struct ewb_field
{
  char *name;
  char *value;
  size_t size;
} ewb_field;

typedef struct ewb_file
{
  char *name;
  char *filename;
  char *content_type;
  char tmp_path[512];
  size_t size;
} ewb_file;

typedef struct ewb_form
{
  char content_type[256];
  char boundary[256];
  char *body;
  size_t body_len;

  ewb_field fields[EWB_MAX_FIELDS];
  int nfields;
  ewb_file files[EWB_MAX_FILES];
  int nfiles;

  char *entrypoint;
  char *stackpos;
  char *stack;
  size_t stack_size;
  char *signature;
} ewb_form;

void ewb_form_init(ewb_form *f);
void ewb_form_free(ewb_form *f);
int  ewb_form_parse(ewb_form *f, const char *content_type, char *body, size_t body_len);

#endif
