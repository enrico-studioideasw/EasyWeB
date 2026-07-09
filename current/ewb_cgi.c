#include "cgi_parser.h"
#include "vm_1.0.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{ FILE *f=fopen(path,"rb");
  if (!f) return NULL;

  if (fseek(f,0,SEEK_END))
  { fclose(f);
    return NULL;
  };

  long n=ftell(f);
  if (n<0)
  { fclose(f);
    return NULL;
  };

  if (fseek(f,0,SEEK_SET))
  { fclose(f);
    return NULL;
  };

  char *buf=(char*)malloc((size_t)n+1);
  if (!buf)
  { fclose(f);
    return NULL;
  };

  size_t r=fread(buf,1,(size_t)n,f);
  fclose(f);
  buf[r]=0;
  return buf;
};

static char *copy_string(const char *s)
{ if (!s) s="";

  char *out=(char*)malloc(strlen(s)+1);
  if (!out) return NULL;

  strcpy(out,s);
  return out;
};

static char *read_stdin_body(size_t *len_out)
{ char *cl=getenv("CONTENT_LENGTH");
  long n=0;

  *len_out=0;

  if (cl && cl[0]) n=atol(cl);
  if (n<=0) return copy_string("");

  char *body=(char*)malloc((size_t)n+1);
  if (!body) return NULL;

  size_t r=fread(body,1,(size_t)n,stdin);
  body[r]=0;
  *len_out=r;
  return body;
};

static const char *program_path(int argc, char **argv)
{ const char *p=NULL;

  if (argc>1) p=argv[1];
  if (!p || !p[0]) p=getenv("EWB_PROGRAM");
  if (!p || !p[0]) p=getenv("PATH_TRANSLATED");

  return p;
};

int main(int argc, char **argv)
{ ewb_form form;
  char *program=NULL;
  char *body=NULL;
  size_t body_len=0;
  const char *ctype=getenv("CONTENT_TYPE");
  const char *method=getenv("REQUEST_METHOD");
  const char *path=NULL;
  const char *stack="";
  int entrypoint=0;
  int stackpos=0;
  int rc=1;

  printf("Content-Type: text/html; charset=utf-8\r\n\r\n");

  path=program_path(argc,argv);
  if (!path || !path[0])
  { printf("Missing EWB program\n");
    return 1;
  };

  program=read_file(path);
  if (!program)
  { printf("Cannot read EWB program\n");
    return 1;
  };

  if (method && !strcmp(method,"POST"))
  { body=read_stdin_body(&body_len);
    if (!body)
    { free(program);
      printf("Cannot read POST body\n");
      return 1;
    };
  }
  else
  { body=copy_string(getenv("QUERY_STRING"));
    if (!body)
    { free(program);
      printf("Cannot read QUERY_STRING\n");
      return 1;
    };
    body_len=strlen(body);
    ctype="application/x-www-form-urlencoded";
  };

  if (ewb_form_parse(&form,ctype,body,body_len))
  { ewb_form_free(&form);
    free(program);
    printf("Bad CGI form\n");
    return 1;
  };

  if (form.entrypoint) entrypoint=atoi(form.entrypoint);
  if (form.stackpos) stackpos=atoi(form.stackpos);
  if (form.stack) stack=form.stack;

  rc=ewb_run_text(program,entrypoint,stackpos,stack);

  ewb_form_free(&form);
  free(program);
  return rc;
};
