#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define EWIA_VERSION "0.1"
#define DEFAULT_API_URL "https://api.openai.com/v1/chat/completions"
#define CONFIG_FILE ".ewIA.conf"
#define RESOURCE_DIR "IA-Resources"
#define RESOURCE_INSTALL_DIR "/usr/local/share/ewb/IA-Resources"

typedef struct
{ char *api_key;
  char *model;
  char *api_url;
} Config;

typedef struct
{ char *data;
  size_t len;
  size_t cap;
} Buf;

typedef struct
{ size_t start;
  size_t end;
  char *original;
  char *request;
  char *comment;
  char *global_code;
  char *local_code;
} AiBlock;

static void die(const char *msg)
{ fprintf(stderr,"ewIA: %s\n",msg);
  exit(1);
};

static void *xmalloc(size_t n)
{ void *p;
  p=malloc(n);
  if (p==NULL) die("memoria esaurita");
  return p;
};

static char *xstrdup(const char *s)
{ char *r;
  r=(char *)xmalloc(strlen(s)+1);
  strcpy(r,s);
  return r;
};

static void buf_init(Buf *b)
{ b->data=NULL;
  b->len=0;
  b->cap=0;
};

static void buf_need(Buf *b,size_t n)
{ char *p;
  size_t nc;

  if (b->len+n+1<=b->cap) return;
  nc=b->cap;
  if (nc==0) nc=1024;
  while (b->len+n+1>nc) nc=nc*2;
  p=(char *)realloc(b->data,nc);
  if (p==NULL) die("memoria esaurita");
  b->data=p;
  b->cap=nc;
};

static void buf_addn(Buf *b,const char *s,size_t n)
{ buf_need(b,n);
  memcpy(b->data+b->len,s,n);
  b->len=b->len+n;
  b->data[b->len]=0;
};

static void buf_add(Buf *b,const char *s)
{ buf_addn(b,s,strlen(s));
};

static void buf_addc(Buf *b,char c)
{ buf_need(b,1);
  b->data[b->len]=c;
  b->len++;
  b->data[b->len]=0;
};

static char *buf_take(Buf *b)
{ char *p;
  if (b->data==NULL) return xstrdup("");
  p=b->data;
  b->data=NULL;
  b->len=0;
  b->cap=0;
  return p;
};

static char *read_file(const char *name,size_t *len)
{ FILE *f;
  char *data;
  long n;

  f=fopen(name,"rb");
  if (f==NULL) return NULL;
  if (fseek(f,0,SEEK_END)!=0) die("seek fallita");
  n=ftell(f);
  if (n<0) die("ftell fallita");
  if (fseek(f,0,SEEK_SET)!=0) die("seek fallita");
  data=(char *)xmalloc((size_t)n+1);
  if (fread(data,1,(size_t)n,f)!=(size_t)n) die("lettura file incompleta");
  fclose(f);
  data[n]=0;
  if (len!=NULL) *len=(size_t)n;
  return data;
};

static void write_file(const char *name,const char *data,size_t len)
{ FILE *f;

  f=fopen(name,"wb");
  if (f==NULL)
  { perror(name);
    die("non posso scrivere il file");
  };
  if (fwrite(data,1,len,f)!=len) die("scrittura file incompleta");
  fclose(f);
};

static char *path_join(const char *a,const char *b)
{ Buf out;
  buf_init(&out);
  buf_add(&out,a);
  if (a[0]!=0 && a[strlen(a)-1]!='/') buf_addc(&out,'/');
  buf_add(&out,b);
  return buf_take(&out);
};

static char *config_path(void)
{ char *home;

  home=getenv("HOME");
  if (home==NULL || home[0]==0) home=".";
  return path_join(home,CONFIG_FILE);
};

static char *trimdup(const char *s)
{ const char *e;

  while (*s && isspace((unsigned char)*s)) s++;
  e=s+strlen(s);
  while (e>s && isspace((unsigned char)e[-1])) e--;
  return strndup(s,(size_t)(e-s));
};

static char *read_line_value(const char *label,const char *def)
{ char line[4096];
  char *v;

  if (def!=NULL && def[0]!=0) printf("%s [%s]: ",label,def);
  else printf("%s: ",label);
  fflush(stdout);
  if (fgets(line,sizeof(line),stdin)==NULL) die("configurazione interrotta");
  v=trimdup(line);
  if (v[0]==0 && def!=NULL)
  { free(v);
    return xstrdup(def);
  };
  return v;
};

static void config_defaults(Config *cfg)
{ cfg->api_key=NULL;
  cfg->model=NULL;
  cfg->api_url=NULL;
};

static int load_config(Config *cfg)
{ char *name;
  char *data;
  char *p;
  char *line;
  size_t len;

  name=config_path();
  data=read_file(name,&len);
  free(name);
  if (data==NULL) return 0;
  p=data;
  while (*p)
  { line=p;
    while (*p && *p!='\n') p++;
    if (*p=='\n')
    { *p=0;
      p++;
    };
    if (strncmp(line,"api_key=",8)==0) cfg->api_key=xstrdup(line+8);
    if (strncmp(line,"model=",6)==0) cfg->model=xstrdup(line+6);
    if (strncmp(line,"api_url=",8)==0) cfg->api_url=xstrdup(line+8);
  };
  free(data);
  if (cfg->api_key==NULL || cfg->model==NULL || cfg->api_url==NULL) return 0;
  return 1;
};

static void save_config(Config *cfg)
{ char *name;
  Buf out;

  name=config_path();
  buf_init(&out);
  buf_add(&out,"api_key=");
  buf_add(&out,cfg->api_key);
  buf_add(&out,"\nmodel=");
  buf_add(&out,cfg->model);
  buf_add(&out,"\napi_url=");
  buf_add(&out,cfg->api_url);
  buf_add(&out,"\n");
  write_file(name,out.data,out.len);
  chmod(name,0600);
  free(name);
  free(out.data);
};

static void ask_config(Config *cfg)
{ printf("Prima configurazione ewIA\n");
  cfg->api_key=read_line_value("API Key OpenAI",NULL);
  cfg->model=read_line_value("Modello","gpt-5-mini");
  cfg->api_url=read_line_value("URL API",DEFAULT_API_URL);
  save_config(cfg);
};

static int is_comment_before(const char *src,size_t pos)
{ size_t p;

  p=pos;
  while (p>0 && src[p-1]!='\n') p--;
  while (p<pos && (src[p]==' ' || src[p]=='\t')) p++;
  if (p<pos && src[p]=='\'') return 1;
  return 0;
};

static int match_ai_call(const char *src,size_t pos,size_t len)
{ size_t p;
  char a;
  char b;

  if (pos+2>=len) return 0;
  a=(char)tolower((unsigned char)src[pos]);
  b=(char)tolower((unsigned char)src[pos+1]);
  if (!((a=='a' && b=='i') || (a=='i' && b=='a'))) return 0;
  if (pos>0 && (isalnum((unsigned char)src[pos-1]) || src[pos-1]=='_')) return 0;
  if (pos+2<len && (isalnum((unsigned char)src[pos+2]) || src[pos+2]=='_')) return 0;
  p=pos+2;
  while (p<len && isspace((unsigned char)src[p])) p++;
  if (p<len && src[p]=='(') return 1;
  return 0;
};

static char *unescape_request(const char *s,size_t len)
{ Buf out;
  size_t i;

  buf_init(&out);
  i=0;
  while (i<len)
  { if (s[i]=='\\' && i+1<len)
    { i++;
      if (s[i]=='n') buf_addc(&out,'\n');
      else if (s[i]=='t') buf_addc(&out,'\t');
      else buf_addc(&out,s[i]);
    }
    else buf_addc(&out,s[i]);
    i++;
  };
  return buf_take(&out);
};

static int find_ai_request(const char *src,size_t len,size_t from,AiBlock *b)
{ size_t p;
  size_t q;
  size_t str_start;
  int esc;

  p=from;
  while (p<len)
  { if (match_ai_call(src,p,len) && !is_comment_before(src,p))
    { q=p+2;
      while (q<len && isspace((unsigned char)src[q])) q++;
      if (q>=len || src[q]!='(') return 0;
      q++;
      while (q<len && isspace((unsigned char)src[q])) q++;
      if (q>=len || src[q]!='"')
      { p++;
        continue;
      };
      q++;
      str_start=q;
      esc=0;
      while (q<len)
      { if (esc) esc=0;
        else if (src[q]=='\\') esc=1;
        else if (src[q]=='"') break;
        q++;
      };
      if (q>=len) die("AI con stringa non chiusa");
      b->start=p;
      b->request=unescape_request(src+str_start,q-str_start);
      q++;
      while (q<len && isspace((unsigned char)src[q])) q++;
      if (q<len && src[q]==')') q++;
      while (q<len && src[q]!='\n')
      { if (src[q]==';')
        { q++;
          break;
        };
        q++;
      };
      b->end=q;
      b->original=strndup(src+b->start,b->end-b->start);
      b->comment=NULL;
      b->global_code=NULL;
      b->local_code=NULL;
      return 1;
    };
    p++;
  };
  return 0;
};

static char *json_escape(const char *s)
{ Buf out;
  const unsigned char *p;
  char tmp[16];

  buf_init(&out);
  p=(const unsigned char *)s;
  while (*p)
  { if (*p=='"' || *p=='\\')
    { buf_addc(&out,'\\');
      buf_addc(&out,(char)*p);
    }
    else if (*p=='\n') buf_add(&out,"\\n");
    else if (*p=='\r') buf_add(&out,"\\r");
    else if (*p=='\t') buf_add(&out,"\\t");
    else if (*p<32)
    { snprintf(tmp,sizeof(tmp),"\\u%04x",*p);
      buf_add(&out,tmp);
    }
    else buf_addc(&out,(char)*p);
    p++;
  };
  return buf_take(&out);
};

static char *json_unescape(const char *s,size_t len)
{ Buf out;
  size_t i;

  buf_init(&out);
  i=0;
  while (i<len)
  { if (s[i]=='\\' && i+1<len)
    { i++;
      if (s[i]=='n') buf_addc(&out,'\n');
      else if (s[i]=='r') buf_addc(&out,'\r');
      else if (s[i]=='t') buf_addc(&out,'\t');
      else if (s[i]=='"' || s[i]=='\\' || s[i]=='/') buf_addc(&out,s[i]);
      else if (s[i]=='u' && i+4<len)
      { buf_addc(&out,'?');
        i=i+4;
      }
      else buf_addc(&out,s[i]);
    }
    else buf_addc(&out,s[i]);
    i++;
  };
  return buf_take(&out);
};

static char *extract_json_string_after(const char *data,const char *key)
{ char *p;
  char *q;
  int esc;

  p=strstr(data,key);
  if (p==NULL) return NULL;
  p=strchr(p,':');
  if (p==NULL) return NULL;
  p++;
  while (*p && isspace((unsigned char)*p)) p++;
  if (*p!='"') return NULL;
  p++;
  q=p;
  esc=0;
  while (*q)
  { if (esc) esc=0;
    else if (*q=='\\') esc=1;
    else if (*q=='"') return json_unescape(p,(size_t)(q-p));
    q++;
  };
  return NULL;
};

static char *read_resource(const char *name,size_t *len)
{ char *path;
  char *data;
  char *envdir;

  envdir=getenv("EWIA_RESOURCES");
  if (envdir!=NULL && envdir[0]!=0)
  { path=path_join(envdir,name);
    data=read_file(path,len);
    free(path);
    if (data!=NULL) return data;
  };

  path=path_join(RESOURCE_DIR,name);
  data=read_file(path,len);
  free(path);
  if (data!=NULL) return data;

  path=path_join(RESOURCE_INSTALL_DIR,name);
  data=read_file(path,len);
  free(path);
  return data;
};

static void append_resource(Buf *prompt,const char *name)
{ char *data;
  size_t len;

  data=read_resource(name,&len);
  if (data!=NULL)
  { buf_add(prompt,"\n\n===== ");
    buf_add(prompt,name);
    buf_add(prompt," =====\n");
    buf_addn(prompt,data,len);
    free(data);
    return;
  };
  fprintf(stderr,"ewIA: risorsa mancante: %s\n",name);
  die("cartella IA-Resources incompleta");
};

static char *context_around(const char *src,size_t len,size_t start,size_t end)
{ size_t a;
  size_t b;
  size_t lines;

  a=start;
  lines=0;
  while (a>0 && lines<40)
  { a--;
    if (src[a]=='\n') lines++;
  };
  b=end;
  lines=0;
  while (b<len && lines<40)
  { if (src[b]=='\n') lines++;
    b++;
  };
  return strndup(src+a,b-a);
};

static char *build_prompt(const char *src,size_t len,AiBlock *b)
{ Buf prompt;
  char *ctx;

  buf_init(&prompt);
  buf_add(&prompt,"Sei Susan, assistente per il linguaggio EWB/easyweb.\n");
  buf_add(&prompt,"Devi proporre codice, ma il codice sara' inserito commentato.\n");
  buf_add(&prompt,"Rispondi solo nel formato richiesto. Nessun testo fuori dai marker.\n");
  buf_add(&prompt,"Il codice deve essere semplice, leggibile, stile EWB/Amethist.\n");
  append_resource(&prompt,"stile_susan.txt");
  append_resource(&prompt,"sintassi_ewb.txt");
  append_resource(&prompt,"vm_primitives.txt");
  append_resource(&prompt,"manuale_base.txt");
  append_resource(&prompt,"funzioni_predefinite.txt");
  append_resource(&prompt,"esempi_buoni.txt");
  buf_add(&prompt,"\n\n===== FORMATO OBBLIGATORIO =====\n");
  buf_add(&prompt,"BEGIN_AI_RESPONSE\n\nCOMMENT:\n...\n\nGLOBAL_CODE:\n...\n\nLOCAL_CODE:\n...\n\nEND_AI_RESPONSE\n");
  ctx=context_around(src,len,b->start,b->end);
  buf_add(&prompt,"\n\n===== CODICE CIRCOSTANTE =====\n");
  buf_add(&prompt,ctx);
  free(ctx);
  buf_add(&prompt,"\n\n===== RICHIESTA UTENTE =====\n");
  buf_add(&prompt,b->request);
  buf_add(&prompt,"\n");
  return buf_take(&prompt);
};

static char *make_request_json(Config *cfg,const char *prompt)
{ Buf json;
  char *m;
  char *p;

  m=json_escape(cfg->model);
  p=json_escape(prompt);
  buf_init(&json);
  buf_add(&json,"{\"model\":\"");
  buf_add(&json,m);
  buf_add(&json,"\",\"messages\":[");
  buf_add(&json,"{\"role\":\"system\",\"content\":\"Rispondi solo nel formato richiesto da ewIA.\"},");
  buf_add(&json,"{\"role\":\"user\",\"content\":\"");
  buf_add(&json,p);
  buf_add(&json,"\"}],\"temperature\":0.2}");
  free(m);
  free(p);
  return buf_take(&json);
};

static void ensure_curl(void)
{ int rc;

  rc=system("command -v curl >/dev/null 2>&1");
  if (rc<0 || !WIFEXITED(rc) || WEXITSTATUS(rc)!=0)
    die("You need to install CURL to use this tool");
};

static char *temp_name(const char *tag)
{ Buf b;

  buf_init(&b);
  buf_add(&b,"/tmp/ewia_");
  buf_add(&b,tag);
  buf_add(&b,"_XXXXXX");
  return buf_take(&b);
};

static char *call_susan(Config *cfg,const char *prompt)
{ char *body_template;
  char *conf_template;
  char *out_template;
  char *json;
  char *response;
  char *content;
  FILE *f;
  int fd;
  int rc;
  char cmd[1024];
  size_t len;

  ensure_curl();
  body_template=temp_name("body");
  conf_template=temp_name("curl");
  out_template=temp_name("out");
  fd=mkstemp(body_template);
  if (fd<0) die("mkstemp body fallita");
  close(fd);
  fd=mkstemp(conf_template);
  if (fd<0) die("mkstemp curl fallita");
  close(fd);
  fd=mkstemp(out_template);
  if (fd<0) die("mkstemp out fallita");
  close(fd);

  json=make_request_json(cfg,prompt);
  write_file(body_template,json,strlen(json));
  free(json);

  f=fopen(conf_template,"wb");
  if (f==NULL) die("non posso preparare curl");
  chmod(conf_template,0600);
  fprintf(f,"url = \"%s\"\n",cfg->api_url);
  fprintf(f,"request = \"POST\"\n");
  fprintf(f,"header = \"Content-Type: application/json\"\n");
  fprintf(f,"header = \"Authorization: Bearer %s\"\n",cfg->api_key);
  fprintf(f,"data = \"@%s\"\n",body_template);
  fprintf(f,"output = \"%s\"\n",out_template);
  fprintf(f,"silent\nshow-error\nfail\n");
  fclose(f);

  snprintf(cmd,sizeof(cmd),"curl --config %s",conf_template);
  rc=system(cmd);
  unlink(body_template);
  unlink(conf_template);
  free(body_template);
  free(conf_template);
  if (rc<0 || !WIFEXITED(rc) || WEXITSTATUS(rc)!=0)
  { unlink(out_template);
    free(out_template);
    die("chiamata a Susan fallita");
  };

  response=read_file(out_template,&len);
  unlink(out_template);
  free(out_template);
  if (response==NULL) die("risposta Susan non leggibile");
  content=extract_json_string_after(response,"\"content\"");
  free(response);
  if (content==NULL) die("risposta API senza content");
  return content;
};

static char *section_between(const char *data,const char *a,const char *b)
{ char *p;
  char *q;

  p=strstr(data,a);
  if (p==NULL) return NULL;
  p=p+strlen(a);
  while (*p=='\r' || *p=='\n') p++;
  q=strstr(p,b);
  if (q==NULL) return NULL;
  while (q>p && (q[-1]=='\n' || q[-1]=='\r' || q[-1]==' ' || q[-1]=='\t')) q--;
  return strndup(p,(size_t)(q-p));
};

static void parse_ai_response(const char *r,AiBlock *b)
{ char *begin;
  char *end;

  begin=strstr(r,"BEGIN_AI_RESPONSE");
  end=strstr(r,"END_AI_RESPONSE");
  if (begin==NULL || end==NULL || begin>end) die("Susan non ha rispettato i marker");
  b->comment=section_between(r,"COMMENT:","GLOBAL_CODE:");
  b->global_code=section_between(r,"GLOBAL_CODE:","LOCAL_CODE:");
  b->local_code=section_between(r,"LOCAL_CODE:","END_AI_RESPONSE");
  if (b->comment==NULL || b->global_code==NULL || b->local_code==NULL)
    die("risposta Susan incompleta");
};

static void add_prefixed_lines(Buf *out,const char *prefix,const char *text)
{ const char *p;
  const char *q;

  p=text;
  while (*p)
  { q=p;
    while (*q && *q!='\n') q++;
    buf_add(out,prefix);
    buf_addn(out,p,(size_t)(q-p));
    buf_addc(out,'\n');
    if (*q=='\n') q++;
    p=q;
  };
};

static void add_ai_metadata(Buf *out,const char *label,const char *text)
{ buf_add(out,"'IA ");
  buf_add(out,label);
  buf_add(out,"\n");
  add_prefixed_lines(out,"'IA ",text);
};

static void add_commented_original(Buf *out,const char *original)
{ add_prefixed_lines(out,"' ",original);
};

static void add_local_proposal(Buf *out,AiBlock *b)
{ buf_add(out,"'IA BEGIN_SUSAN_PROPOSAL\n");
  add_ai_metadata(out,"REQUEST:",b->original);
  add_ai_metadata(out,"COMMENT:",b->comment);
  buf_add(out,"'IA LOCAL_CODE:\n");
  add_prefixed_lines(out,"' ",b->local_code);
  buf_add(out,"'IA END_SUSAN_PROPOSAL\n");
};

static void add_global_proposal(Buf *out,AiBlock *b)
{ if (b->global_code==NULL || b->global_code[0]==0) return;
  buf_add(out,"'IA BEGIN_SUSAN_GLOBAL_CODE\n");
  add_ai_metadata(out,"REQUEST:",b->original);
  add_ai_metadata(out,"COMMENT:",b->comment);
  add_prefixed_lines(out,"' ",b->global_code);
  buf_add(out,"'IA END_SUSAN_GLOBAL_CODE\n\n");
};

static char *rewrite_source(const char *src,size_t len,AiBlock *blocks,int n)
{ Buf out;
  Buf globals;
  size_t pos;
  int i;

  buf_init(&out);
  buf_init(&globals);
  for (i=0;i<n;i++) add_global_proposal(&globals,&blocks[i]);
  if (globals.len>0)
  { buf_add(&out,"'IA BEGIN_SUSAN_GLOBAL_PROPOSALS\n");
    buf_addn(&out,globals.data,globals.len);
    buf_add(&out,"'IA END_SUSAN_GLOBAL_PROPOSALS\n\n");
  };
  free(globals.data);

  pos=0;
  for (i=0;i<n;i++)
  { buf_addn(&out,src+pos,blocks[i].start-pos);
    add_commented_original(&out,blocks[i].original);
    if (blocks[i].end<len && src[blocks[i].end]=='\n') buf_addc(&out,'\n');
    else buf_addc(&out,'\n');
    add_local_proposal(&out,&blocks[i]);
    pos=blocks[i].end;
  };
  buf_addn(&out,src+pos,len-pos);
  return buf_take(&out);
};

static char *output_name(const char *src,int dry_run)
{ if (!dry_run) return xstrdup(src);
  Buf out;
  buf_init(&out);
  buf_add(&out,src);
  buf_add(&out,".new");
  return buf_take(&out);
};

static void backup_file(const char *src)
{ Buf bak;
  char *data;
  size_t len;

  data=read_file(src,&len);
  if (data==NULL) die("sorgente non leggibile per backup");
  buf_init(&bak);
  buf_add(&bak,src);
  buf_add(&bak,".bak");
  write_file(bak.data,data,len);
  free(data);
  free(bak.data);
};

static void process_source(Config *cfg,const char *name,int dry_run)
{ char *src;
  char *prompt;
  char *answer;
  char *newsrc;
  char *outname;
  size_t len;
  size_t pos;
  int count;
  int cap;
  AiBlock *blocks;
  AiBlock b;

  src=read_file(name,&len);
  if (src==NULL)
  { perror(name);
    die("sorgente non leggibile");
  };

  count=0;
  cap=4;
  blocks=(AiBlock *)xmalloc(sizeof(AiBlock)*(size_t)cap);
  pos=0;
  while (find_ai_request(src,len,pos,&b))
  { if (count>=cap)
    { cap=cap*2;
      blocks=(AiBlock *)realloc(blocks,sizeof(AiBlock)*(size_t)cap);
      if (blocks==NULL) die("memoria esaurita");
    };
    printf("%s: richiesta AI trovata\n",name);
    prompt=build_prompt(src,len,&b);
    answer=call_susan(cfg,prompt);
    free(prompt);
    parse_ai_response(answer,&b);
    free(answer);
    blocks[count]=b;
    count++;
    pos=b.end;
  };

  if (count==0)
  { printf("%s: nessuna richiesta AI da elaborare\n",name);
    free(blocks);
    free(src);
    return;
  };

  newsrc=rewrite_source(src,len,blocks,count);
  outname=output_name(name,dry_run);
  if (!dry_run) backup_file(name);
  write_file(outname,newsrc,strlen(newsrc));
  printf("%s: scritto %s\n",name,outname);
  free(outname);
  free(newsrc);
  free(src);
  free(blocks);
};

static void usage(void)
{ printf("ewIA " EWIA_VERSION "\n");
  printf("uso: ewIA [--dry-run] sorgente.ewb [sorgente2.ewb ...]\n");
};

int main(int argc,char **argv)
{ Config cfg;
  int dry_run;
  int i;
  int files;

  config_defaults(&cfg);
  dry_run=0;
  files=0;
  if (argc<2)
  { usage();
    return 1;
  };
  if (!load_config(&cfg)) ask_config(&cfg);
  for (i=1;i<argc;i++)
  { if (strcmp(argv[i],"--dry-run")==0) dry_run=1;
    else files++;
  };
  if (files==0)
  { usage();
    return 1;
  };
  for (i=1;i<argc;i++)
  { if (strcmp(argv[i],"--dry-run")!=0) process_source(&cfg,argv[i],dry_run);
  };
  return 0;
};
