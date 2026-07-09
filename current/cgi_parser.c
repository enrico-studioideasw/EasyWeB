#define _GNU_SOURCE

#include "cgi_parser.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *xstrndup(const char *s, size_t n)
{ char *out=(char*)malloc(n+1);
  if (!out) return NULL;
  memcpy(out,s,n);
  out[n]=0;
  return out;
};

void ewb_form_init(ewb_form *f)
{ memset(f,0,sizeof(*f));
};

void ewb_form_free(ewb_form *f)
{ for (int i=0; i<f->nfields; i++)
  { free(f->fields[i].name);
    free(f->fields[i].value);
  }
  for (int i=0; i<f->nfiles; i++)
  { if (f->files[i].tmp_path[0]) unlink(f->files[i].tmp_path);
    free(f->files[i].name);
    free(f->files[i].filename);
    free(f->files[i].content_type);
  }
  free(f->body);
  memset(f,0,sizeof(*f));
};

static char *mem_find(const char *hay, size_t haylen, const char *needle, size_t nlen)
{ if (nlen==0 || haylen<nlen) return NULL;
  for (size_t i=0; i+nlen<=haylen; i++)
  { if (!memcmp(hay+i,needle,nlen)) return (char*)(hay+i);
  };
  return NULL;
};

static int extract_boundary(const char *content_type, char *boundary, size_t bsz)
{ const char *p=strstr(content_type,"boundary=");
  if (!p) return 0;
  p+=9;

  if (*p=='"')
  { p++;
    size_t i=0;
    while (*p && *p!='"' && i+1<bsz) boundary[i++]=*p++;
    boundary[i]=0;
    return i>0;
  };

  size_t i=0;
  while (*p && *p!=';' && *p!=' ' && *p!='\t' && i+1<bsz) boundary[i++]=*p++;
  boundary[i]=0;
  return i>0;
};

static int header_value(const char *headers, const char *name, char *out, size_t outsz)
{ size_t nlen=strlen(name);
  const char *p=headers;
  out[0]=0;

  while (*p)
  { const char *line=p;
    const char *e=strstr(line,"\n");
    size_t len;
    if (e)
    { len=(size_t)(e-line);
    }
    else
    { len=strlen(line);
    };
    while (len>0 && (line[len-1]=='\r' || line[len-1]=='\n')) len--;

    if (len>nlen && !strncasecmp(line,name,nlen) && line[nlen]==':')
    { const char *v=line+nlen+1;
      while ((size_t)(v-line)<len && (*v==' ' || *v=='\t')) v++;
      size_t vlen=len-(size_t)(v-line);
      if (vlen>=outsz) vlen=outsz-1;
      memcpy(out,v,vlen);
      out[vlen]=0;
      return 1;
    };

    if (!e) break;
    p=e+1;
  };
  return 0;
};

static int header_param(const char *line, const char *name, char *out, size_t outsz)
{ char pattern[128];
  snprintf(pattern,sizeof(pattern),"%s=",name);
  const char *p=strstr(line,pattern);
  if (!p) return 0;
  p+=strlen(pattern);

  if (*p=='"')
  { p++;
    size_t i=0;
    while (*p && *p!='"' && i+1<outsz) out[i++]=*p++;
    out[i]=0;
    return i>0;
  };

  size_t i=0;
  while (*p && *p!=';' && *p!=' ' && *p!='\t' && i+1<outsz) out[i++]=*p++;
  out[i]=0;
  return i>0;
};

static void add_field(ewb_form *f, const char *name, const char *value, size_t size)
{ if (f->nfields>=EWB_MAX_FIELDS) return;
  ewb_field *p=&f->fields[f->nfields++];
  p->name=xstrndup(name,strlen(name));
  p->value=xstrndup(value,size);
  p->size=size;

  if (!strcmp(name,"__entrypoint")) f->entrypoint=p->value;
  else if (!strcmp(name,"__stackpos")) f->stackpos=p->value;
  else if (!strcmp(name,"__stack"))
  { f->stack=p->value;
    f->stack_size=p->size;
  }
  else if (!strcmp(name,"__signature")) f->signature=p->value;
};

static int add_file(ewb_form *f, const char *name, const char *filename,
                    const char *content_type, const char *data, size_t size)
{ if (f->nfiles>=EWB_MAX_FILES) return -1;

  char tmp_path[512];
  snprintf(tmp_path,sizeof(tmp_path),"/tmp/ewb-upload-XXXXXX");
  int fd=mkstemp(tmp_path);
  if (fd<0) return -1;

  size_t done=0;
  while (done<size)
  { ssize_t n=write(fd,data+done,size-done);
    if (n<0)
    { close(fd);
      unlink(tmp_path);
      return -1;
    };
    done+=(size_t)n;
  };
  close(fd);

  ewb_file *p=&f->files[f->nfiles++];
  p->name=xstrndup(name,strlen(name));
  p->filename=xstrndup(filename,strlen(filename));
  p->content_type=xstrndup(content_type,strlen(content_type));
  snprintf(p->tmp_path,sizeof(p->tmp_path),"%s",tmp_path);
  p->size=size;
  return 0;
};

static int parse_multipart(ewb_form *f)
{ char marker[320];
  snprintf(marker,sizeof(marker),"--%s",f->boundary);
  size_t marker_len=strlen(marker);

  char *p=mem_find(f->body,f->body_len,marker,marker_len);
  if (!p) return -1;
  char *end_body=f->body+f->body_len;

  for (;;)
  { p+=marker_len;
    if (p+2<=end_body && p[0]=='-' && p[1]=='-') break;
    if (p+2<=end_body && p[0]=='\r' && p[1]=='\n') p+=2;
    else if (p<end_body && p[0]=='\n') p++;
    else break;

    char *headers_end=mem_find(p,(size_t)(end_body-p),"\r\n\r\n",4);
    int sep_len=4;
    if (!headers_end)
    { headers_end=mem_find(p,(size_t)(end_body-p),"\n\n",2);
      sep_len=2;
    };
    if (!headers_end) return -1;

    char *part_data=headers_end+sep_len;
    char *next=mem_find(part_data,(size_t)(end_body-part_data),marker,marker_len);
    if (!next) return -1;

    char *part_end=next;
    if (part_end-2>=part_data && part_end[-2]=='\r' && part_end[-1]=='\n') part_end-=2;
    else if (part_end-1>=part_data && part_end[-1]=='\n') part_end-=1;

    char *headers=xstrndup(p,(size_t)(headers_end-p));
    if (!headers) return -1;

    char disp[1024];
    char name[256];
    char filename[512];
    char ctype[256];
    disp[0]=name[0]=filename[0]=ctype[0]=0;
    header_value(headers,"Content-Disposition",disp,sizeof(disp));
    header_value(headers,"Content-Type",ctype,sizeof(ctype));
    header_param(disp,"name",name,sizeof(name));
    header_param(disp,"filename",filename,sizeof(filename));
    if (ctype[0]==0) strcpy(ctype,"application/octet-stream");

    if (name[0])
    { if (filename[0]) add_file(f,name,filename,ctype,part_data,(size_t)(part_end-part_data));
      else add_field(f,name,part_data,(size_t)(part_end-part_data));
    };
    free(headers);
    p=next;
  };
  return 0;
};

static int hexval(char c)
{ if (c>='0' && c<='9') return c-'0';
  if (c>='a' && c<='f') return c-'a'+10;
  if (c>='A' && c<='F') return c-'A'+10;
  return -1;
};

static char *url_decode(const char *s, size_t n)
{ char *out=(char*)malloc(n+1);
  if (!out) return NULL;
  size_t j=0;
  for (size_t i=0; i<n; i++)
  { if (s[i]=='+') out[j++]=' ';
    else if (s[i]=='%' && i+2<n && hexval(s[i+1])>=0 && hexval(s[i+2])>=0)
    { out[j++]=(char)((hexval(s[i+1])<<4)|hexval(s[i+2]));
      i+=2;
    }
    else out[j++]=s[i];
  };
  out[j]=0;
  return out;
};

static int parse_urlencoded(ewb_form *f)
{ char *p=f->body;
  char *end=f->body+f->body_len;

  while (p<end)
  { char *amp=memchr(p,'&',(size_t)(end-p));
    char *item_end;
    if (amp)
    { item_end=amp;
    }
    else
    { item_end=end;
    };
    char *eq=memchr(p,'=',(size_t)(item_end-p));

    if (eq)
    { char *name=url_decode(p,(size_t)(eq-p));
      char *value=url_decode(eq+1,(size_t)(item_end-eq-1));
      if (!name || !value)
      { free(name);
        free(value);
        return -1;
      };
      add_field(f,name,value,strlen(value));
      free(name);
      free(value);
    };
    if (amp)
    { p=amp+1;
    }
    else
    { p=end;
    };
  };
  return 0;
};

int ewb_form_parse(ewb_form *f, const char *content_type, char *body, size_t body_len)
{ ewb_form_init(f);
  if (content_type) snprintf(f->content_type,sizeof(f->content_type),"%s",content_type);
  f->body=body;
  f->body_len=body_len;

  if (!strncmp(f->content_type,"multipart/form-data",19))
  { if (!extract_boundary(f->content_type,f->boundary,sizeof(f->boundary))) return -1;
    return parse_multipart(f);
  };

  if (!strncmp(f->content_type,"application/x-www-form-urlencoded",33))
    return parse_urlencoded(f);

  if (body_len==0) return 0;
  return -1;
};
