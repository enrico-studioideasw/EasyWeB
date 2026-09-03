/*
 * ewbd - EasyWeb daemon, first process/socket skeleton.
 *
 * Coordinator:
 *   - listens HTTP
 *   - keeps a pool of responder processes
 *   - passes accepted client sockets to responders via SCM_RIGHTS
 *
 * Responder:
 *   - receives one client fd
 *   - handles one HTTP request, closes the connection
 *   - keeps program cache space in-process (not implemented yet)
 *
 * This is intentionally plain C: no external libraries, no TLS, no keepalive.
 */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "cgi_parser.h"
#include "cron_db.h"
#include "cron_parser.h"
#include "vm.h"

#define EWBD_MAX_RESPONDERS 1024
#define EWBD_REQBUF 16384
#define EWBD_MAX_POST (10*1024*1024)
#define EWBD_CTRL_PAYLOAD_MAX 4096
#define EWBD_CLIENT_READ_TIMEOUT_SEC 30
#define EWBD_CGI_TIMEOUT_DEFAULT_SEC 120
#define EWBD_CGI_MAX_RESPONSE (1024*1024)

typedef struct control_msg
{ int generation;
  int payload_len;
  char tag;
} control_msg;

typedef struct request_stat
{ uint64_t elapsed_ns;
  int is_evm;
  int is_error;
} request_stat;

typedef struct traffic_counters
{ uint64_t frames;
  uint64_t errors;
  uint64_t elapsed_ns;
} traffic_counters;

typedef struct responder
{
  pid_t pid;
  int fd;
  int busy;
  int generation;
} responder;

static volatile sig_atomic_t g_hup=0;
static volatile sig_atomic_t g_chld=0;
static volatile sig_atomic_t g_stop=0;
static responder g_resp[EWBD_MAX_RESPONDERS];
static int g_num_resp=0;
static int g_min_resp=2;
static int g_max_resp=16;
static int g_generation=1;
static int g_cgi_timeout_sec=EWBD_CGI_TIMEOUT_DEFAULT_SEC;
static char g_www_root[4096]=".";
static char g_stats_file[4096]="ewbd-traffic.log";
static traffic_counters g_evm_stats;
static traffic_counters g_other_stats;
static time_t g_stats_started=0;

static void on_stop(int sig)
{
  (void)sig;
  g_stop=1;
}

static void die(const char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
  fprintf(stderr,"\n");
  exit(1);
}

static void warnx(const char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
  fprintf(stderr,"\n");
}

static void on_hup(int sig)
{
  (void)sig;
  g_hup=1;
}

static void on_chld(int sig)
{
  (void)sig;
  g_chld=1;
}

static uint64_t elapsed_ns(struct timespec start, struct timespec end)
{
  uint64_t seconds=(uint64_t)(end.tv_sec-start.tv_sec);
  long nanoseconds=end.tv_nsec-start.tv_nsec;
  if (nanoseconds<0)
  {
    seconds--;
    nanoseconds+=1000000000L;
  }
  return seconds*1000000000ULL+(uint64_t)nanoseconds;
}

static void add_request_stat(const request_stat *stat)
{
  traffic_counters *counter=stat->is_evm ? &g_evm_stats : &g_other_stats;
  counter->frames++;
  counter->elapsed_ns+=stat->elapsed_ns;
  if (stat->is_error) counter->errors++;
}

static int write_traffic_counters(time_t now)
{
  FILE *out;
  struct tm tmv;
  char timestamp[32];
  double evm_average=g_evm_stats.frames ?
    (double)g_evm_stats.elapsed_ns/(double)g_evm_stats.frames/1000000.0 : 0.0;
  double other_average=g_other_stats.frames ?
    (double)g_other_stats.elapsed_ns/(double)g_other_stats.frames/1000000.0 : 0.0;

  if (!localtime_r(&now,&tmv)) return -1;
  if (!strftime(timestamp,sizeof(timestamp),"%Y-%m-%d %H:%M:%S",&tmv)) return -1;

  out=fopen(g_stats_file,"a");
  if (!out) return -1;
  fprintf(out,
          "%s EVM frames %llu, %llu errors, avg %.1fms, "
          "OTHERS %llu, %llu errors, avg %.1fms\n",
          timestamp,
          (unsigned long long)g_evm_stats.frames,
          (unsigned long long)g_evm_stats.errors,
          evm_average,
          (unsigned long long)g_other_stats.frames,
          (unsigned long long)g_other_stats.errors,
          other_average);
  if (fclose(out)<0) return -1;

  memset(&g_evm_stats,0,sizeof(g_evm_stats));
  memset(&g_other_stats,0,sizeof(g_other_stats));
  g_stats_started=now;
  return 0;
}

static void traffic_tick(void)
{
  time_t now=time(NULL);
  if (g_stats_started==0) g_stats_started=now;
  if (now-g_stats_started<300) return;
  if (write_traffic_counters(now)<0)
    warnx("cannot write traffic counters to %s: %s",g_stats_file,strerror(errno));
}

static int set_cloexec(int fd)
{
  int flags=fcntl(fd,F_GETFD,0);
  if (flags<0) return -1;
  return fcntl(fd,F_SETFD,flags|FD_CLOEXEC);
}

static int send_ctrl_msg(int sock, char tag, int fd_to_send, int generation, const char *payload, int payload_len)
{
  control_msg hdr;
  struct iovec iov[2];
  struct msghdr msg;
  char control[CMSG_SPACE(sizeof(int))];

  hdr.generation=generation;
  hdr.payload_len=payload_len;
  hdr.tag=tag;

  memset(&msg,0,sizeof(msg));
  iov[0].iov_base=&hdr;
  iov[0].iov_len=sizeof(hdr);
  msg.msg_iov=iov;
  msg.msg_iovlen=1;

  if (payload_len>0)
  {
    iov[1].iov_base=(void*)payload;
    iov[1].iov_len=(size_t)payload_len;
    msg.msg_iovlen=2;
  }

  if (fd_to_send>=0)
  {
    memset(control,0,sizeof(control));
    msg.msg_control=control;
    msg.msg_controllen=sizeof(control);

    struct cmsghdr *cmsg=CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level=SOL_SOCKET;
    cmsg->cmsg_type=SCM_RIGHTS;
    cmsg->cmsg_len=CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg),&fd_to_send,sizeof(int));
  }

  if (sendmsg(sock,&msg,0)!=(ssize_t)(sizeof(hdr)+payload_len)) return -1;
  return 0;
}

static int recv_ctrl_msg(int sock, char *tag, int *fd_out, int *generation, char *payload, int payload_sz, int *payload_len)
{
  char data[sizeof(control_msg)+EWBD_CTRL_PAYLOAD_MAX];
  control_msg hdr;
  struct iovec iov;
  struct msghdr msg;
  char control[CMSG_SPACE(sizeof(int))];
  ssize_t n;

  *fd_out=-1;
  *generation=0;
  *payload_len=0;

  memset(&msg,0,sizeof(msg));
  memset(control,0,sizeof(control));
  iov.iov_base=data;
  iov.iov_len=sizeof(data);
  msg.msg_iov=&iov;
  msg.msg_iovlen=1;
  msg.msg_control=control;
  msg.msg_controllen=sizeof(control);

  n=recvmsg(sock,&msg,0);
  if (n<=0) return -1;
  if (n<(ssize_t)sizeof(hdr)) return -1;

  memcpy(&hdr,data,sizeof(hdr));
  if (hdr.payload_len<0 || hdr.payload_len>EWBD_CTRL_PAYLOAD_MAX) return -1;
  if (hdr.payload_len>=payload_sz) return -1;
  if (n<(ssize_t)(sizeof(hdr)+hdr.payload_len)) return -1;

  *tag=hdr.tag;
  *generation=hdr.generation;
  *payload_len=hdr.payload_len;
  if (hdr.payload_len>0)
  {
    memcpy(payload,data+sizeof(hdr),(size_t)hdr.payload_len);
    payload[hdr.payload_len]=0;
  }
  else if (payload_sz>0) payload[0]=0;

  struct cmsghdr *cmsg=CMSG_FIRSTHDR(&msg);
  if (cmsg && cmsg->cmsg_level==SOL_SOCKET && cmsg->cmsg_type==SCM_RIGHTS)
    memcpy(fd_out,CMSG_DATA(cmsg),sizeof(int));

  return 0;
}

static int send_status(int sock, char tag)
{
  return send_ctrl_msg(sock,tag,-1,0,NULL,0);
}

static int read_http_request(int fd, char *buf, size_t bufsz)
{
  size_t used=0;
  while (used+1<bufsz)
  {
    ssize_t n=read(fd,buf+used,bufsz-used-1);
    if (n<0)
    {
      if (errno==EINTR) continue;
      return -1;
    }
    if (n==0) break;
    used+=(size_t)n;
    buf[used]=0;
    if (strstr(buf,"\r\n\r\n") || strstr(buf,"\n\n")) break;
  }
  buf[used]=0;
  return (int)used;
}

static char *header_end(char *req)
{
  char *p=strstr(req,"\r\n\r\n");
  if (p) return p+4;
  p=strstr(req,"\n\n");
  if (p) return p+2;
  return NULL;
}

static int header_value(const char *headers, const char *name, char *out, size_t outsz)
{
  size_t nlen=strlen(name);
  const char *p=headers;

  out[0]=0;

  while (*p)
  {
    const char *line=p;
    const char *e=strstr(line,"\n");
    size_t len;
    if (e)
    { len=(size_t)(e-line);
    }
    else
    { len=strlen(line);
    }
    while (len>0 && (line[len-1]=='\r' || line[len-1]=='\n')) len--;

    if (len>nlen && !strncasecmp(line,name,nlen) && line[nlen]==':')
    {
      const char *v=line+nlen+1;
      while ((size_t)(v-line)<len && (*v==' ' || *v=='\t')) v++;
      size_t vlen=len-(size_t)(v-line);
      if (vlen>=outsz) vlen=outsz-1;
      memcpy(out,v,vlen);
      out[vlen]=0;
      return 1;
    }

    if (!e) break;
    p=e+1;
  }

  return 0;
}

static int read_http_body(int fd, char *req, int req_len, char **body_out, size_t *body_len_out)
{
  char clen[64];
  char *bend=header_end(req);
  long content_length=0;
  size_t already=0;

  *body_out=NULL;
  *body_len_out=0;

  if (!bend) return -1;

  if (!header_value(req,"Content-Length",clen,sizeof(clen))) return 0;
  content_length=atol(clen);
  if (content_length<0 || content_length>EWBD_MAX_POST) return -1;
  if (content_length==0) return 0;

  char *body=(char*)malloc((size_t)content_length+1);
  if (!body) return -1;

  already=(size_t)(req+req_len-bend);
  if (already>(size_t)content_length) already=(size_t)content_length;
  if (already>0) memcpy(body,bend,already);

  size_t used=already;
  while (used<(size_t)content_length)
  {
    ssize_t n=read(fd,body+used,(size_t)content_length-used);
    if (n<0)
    {
      if (errno==EINTR) continue;
      free(body);
      return -1;
    }
    if (n==0)
    {
      free(body);
      return -1;
    }
    used+=(size_t)n;
  }

  body[content_length]=0;
  *body_out=body;
  *body_len_out=(size_t)content_length;
  return 0;
}

static void parse_request_line(const char *req, char *method, size_t msz, char *path, size_t psz)
{
  method[0]=0;
  path[0]=0;
  sscanf(req,"%15s %1023s",method,path);
  method[msz-1]=0;
  path[psz-1]=0;
}

static void write_all_n(int fd, const char *s, size_t length)
{
  size_t left=length;
  const char *p=s;
  while (left>0)
  {
    ssize_t n=write(fd,p,left);
    if (n<0)
    {
      if (errno==EINTR) continue;
      return;
    }
    p+=n;
    left-=(size_t)n;
  }
}

static void write_all(int fd, const char *s)
{
  write_all_n(fd,s,strlen(s));
}

static int cgi_status_header(const char *line, size_t length)
{
  return length>=7 && !strncasecmp(line,"Status:",7);
}

static int parse_cgi_status(const char *headers, const char *headers_end,
                            int *status_code, char *reason,
                            size_t reason_size)
{
  const char *line=headers;
  int found=0;
  *status_code=200;
  snprintf(reason,reason_size,"OK");

  while (line<headers_end)
  {
    const char *next=memchr(line,'\n',(size_t)(headers_end-line));
    const char *line_end=next ? next : headers_end;
    size_t length=(size_t)(line_end-line);
    if (length>0 && line[length-1]=='\r') length--;

    if (cgi_status_header(line,length))
    {
      char value[512];
      char *p;
      char *number_end;
      long code;
      size_t value_len;
      if (found || length<=7 || length-7>=sizeof(value)) return -1;
      value_len=length-7;
      memcpy(value,line+7,value_len);
      value[value_len]=0;
      p=value;
      while (*p==' ' || *p=='\t') p++;
      errno=0;
      code=strtol(p,&number_end,10);
      if (errno || number_end==p || code<100 || code>599) return -1;
      if (*number_end!=' ' && *number_end!='\t' && *number_end!=0) return -1;
      while (*number_end==' ' || *number_end=='\t') number_end++;
      if (*number_end) snprintf(reason,reason_size,"%s",number_end);
      else snprintf(reason,reason_size,"CGI Status");
      *status_code=(int)code;
      found=1;
    }
    line=next ? next+1 : headers_end;
  }
  return 0;
}

static const char *mime_type(const char *path)
{
  const char *ext=strrchr(path,'.');
  if (!ext) return "application/octet-stream";
  if (!strcmp(ext,".html") || !strcmp(ext,".htm")) return "text/html; charset=utf-8";
  if (!strcmp(ext,".css")) return "text/css; charset=utf-8";
  if (!strcmp(ext,".js")) return "application/javascript; charset=utf-8";
  if (!strcmp(ext,".json")) return "application/json; charset=utf-8";
  if (!strcmp(ext,".png")) return "image/png";
  if (!strcmp(ext,".jpg") || !strcmp(ext,".jpeg")) return "image/jpeg";
  if (!strcmp(ext,".gif")) return "image/gif";
  if (!strcmp(ext,".webp")) return "image/webp";
  if (!strcmp(ext,".svg")) return "image/svg+xml";
  if (!strcmp(ext,".ico")) return "image/x-icon";
  if (!strcmp(ext,".txt")) return "text/plain; charset=utf-8";
  return "application/octet-stream";
}

static int is_static_path(const char *path)
{
  const char *ext=strrchr(path,'.');
  if (!ext) return 0;
  return !strcmp(ext,".html") || !strcmp(ext,".htm") ||
         !strcmp(ext,".css")  || !strcmp(ext,".js")  ||
         !strcmp(ext,".json") ||
         !strcmp(ext,".png")  || !strcmp(ext,".jpg") ||
         !strcmp(ext,".jpeg") || !strcmp(ext,".gif") ||
         !strcmp(ext,".webp") || !strcmp(ext,".svg") ||
         !strcmp(ext,".ico")  ||
         !strcmp(ext,".txt");
}

static int is_evm_path(const char *path)
{
  char clean[1024];
  const char *ext;
  size_t i=0;
  while (path[i] && path[i]!='?' && path[i]!='#' && i+1<sizeof(clean))
  { clean[i]=path[i];
    i++;
  }
  clean[i]=0;
  ext=strrchr(clean,'.');
  if (!ext) return 0;
  return !strcmp(ext,".evm");
}

static int is_cgi_path(const char *path)
{
  char clean[1024];
  const char *ext;
  size_t i=0;
  while (path[i] && path[i]!='?' && path[i]!='#' && i+1<sizeof(clean))
  { clean[i]=path[i];
    i++;
  }
  clean[i]=0;
  ext=strrchr(clean,'.');
  if (!ext) return 0;
  return !strcmp(ext,".cgi");
}

static int unsafe_path(const char *path)
{
  if (path[0]!='/') return 1;
  if (strstr(path,"..")) return 1;
  if (strchr(path,'\\')) return 1;
  return 0;
}

static void strip_query(const char *path, char *out, size_t outsz)
{
  size_t i=0;
  while (path[i] && path[i]!='?' && path[i]!='#' && i+1<outsz)
  { out[i]=path[i];
    i++;
  }
  out[i]=0;
}

static void default_index_path(const char *path, char *out, size_t outsz)
{
  char clean[1024];
  char full[8192];
  const char *query;
  const char *indexes[]={"index.html","index.evm","index.cgi"};
  struct stat st;

  strip_query(path,clean,sizeof(clean));
  if (unsafe_path(clean))
  { snprintf(out,outsz,"%s",path);
    return;
  }

  snprintf(full,sizeof(full),"%s%s",g_www_root,clean);
  if (stat(full,&st)<0 || !S_ISDIR(st.st_mode))
  { snprintf(out,outsz,"%s",path);
    return;
  }

  query=strchr(path,'?');
  for (size_t i=0; i<sizeof(indexes)/sizeof(indexes[0]); i++)
  {
    snprintf(full,sizeof(full),"%s%s%s%s",g_www_root,clean,
             clean[strlen(clean)-1]=='/' ? "" : "/",indexes[i]);
    if (!stat(full,&st) && S_ISREG(st.st_mode))
    {
      const char *slash=clean[strlen(clean)-1]=='/' ? "" : "/";
      const char *suffix=query ? query : "";
      size_t needed=strlen(clean)+strlen(slash)+strlen(indexes[i])+strlen(suffix)+1;
      if (needed>outsz) continue;
      out[0]=0;
      strcat(out,clean);
      strcat(out,slash);
      strcat(out,indexes[i]);
      strcat(out,suffix);
      return;
    }
  }

  snprintf(out,outsz,"%s",path);
}

static void default_evm_path(const char *path, char *out, size_t outsz)
{
  char clean[1024];
  char full[8192];
  const char *base;
  const char *suffix;
  struct stat st;

  strip_query(path,clean,sizeof(clean));
  if (unsafe_path(clean))
  { snprintf(out,outsz,"%s",path);
    return;
  }

  base=strrchr(clean,'/');
  base=base ? base+1 : clean;
  if (!base[0] || strchr(base,'.'))
  { snprintf(out,outsz,"%s",path);
    return;
  }

  snprintf(full,sizeof(full),"%s%s.evm",g_www_root,clean);
  if (stat(full,&st)<0 || !S_ISREG(st.st_mode))
  { snprintf(out,outsz,"%s",path);
    return;
  }

  suffix=strpbrk(path,"?#");
  if (strlen(clean)+4+(suffix ? strlen(suffix) : 0)+1>outsz)
  { snprintf(out,outsz,"%s",path);
    return;
  }
  out[0]=0;
  strcat(out,clean);
  strcat(out,".evm");
  if (suffix) strcat(out,suffix);
}

static void http_error(int fd, int code, const char *text)
{
  char buf[512];
  char body[256];
  snprintf(body,sizeof(body),"%d %s\n",code,text);
  snprintf(buf,sizeof(buf),
           "HTTP/1.0 %d %s\r\n"
           "Content-Type: text/plain; charset=utf-8\r\n"
           "Content-Length: %lu\r\n"
           "Connection: close\r\n"
           "\r\n",
           code,text,(unsigned long)strlen(body));
  write_all(fd,buf);
  write_all(fd,body);
}

static int serve_static_file(int client_fd, const char *request_path, int send_body)
{
  char clean[1024];
  char full[8192];
  char header[512];
  struct stat st;

  strip_query(request_path,clean,sizeof(clean));
  if (!is_static_path(clean)) return 0;

  if (unsafe_path(clean))
  { http_error(client_fd,403,"Forbidden");
    return -1;
  }

  snprintf(full,sizeof(full),"%s%s",g_www_root,clean);

  if (stat(full,&st)<0 || !S_ISREG(st.st_mode))
  { http_error(client_fd,404,"Not Found");
    return -1;
  }

  int f=open(full,O_RDONLY);
  if (f<0)
  { http_error(client_fd,403,"Forbidden");
    return -1;
  }

  snprintf(header,sizeof(header),
           "HTTP/1.0 200 OK\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %lu\r\n"
           "Connection: close\r\n"
           "\r\n",
           mime_type(clean),(unsigned long)st.st_size);
  write_all(client_fd,header);

  if (!send_body)
  { close(f);
    return 1;
  }

  char buf[16384];
  for (;;)
  {
    ssize_t n=read(f,buf,sizeof(buf));
    if (n<0)
    {
      if (errno==EINTR) continue;
      break;
    }
    if (n==0) break;

    ssize_t done=0;
    while (done<n)
    {
      ssize_t w=write(client_fd,buf+done,(size_t)(n-done));
      if (w<0)
      {
        if (errno==EINTR) continue;
        close(f);
        return -1;
      }
      done+=w;
    }
  }
  close(f);
  return 1;
}

// Legge un file programma preservando eventuali byte 0x00 del formato binario.
static char *read_program_file(const char *full, size_t *len_out)
{
  int f=open(full,O_RDONLY);
  struct stat st;
  char *buf;
  size_t done=0;

  *len_out=0;
  if (f<0) return NULL;
  if (fstat(f,&st)<0 || !S_ISREG(st.st_mode))
  { close(f);
    return NULL;
  }

  buf=(char*)malloc((size_t)st.st_size + 1);
  if (!buf)
  { close(f);
    return NULL;
  }

  while (done<(size_t)st.st_size)
  {
    ssize_t n=read(f,buf+done,(size_t)st.st_size-done);
    if (n<0)
    {
      if (errno==EINTR) continue;
      free(buf);
      close(f);
      return NULL;
    }
    if (n==0) break;
    done+=(size_t)n;
  }

  close(f);
  buf[done]=0;
  *len_out=done;
  return buf;
}

static int count_stack_items(const char *stack)
{
  int count=0;
  const char *p=stack;
  int in_token=0;

  if (!p) return 0;
  while (*p)
  {
    if (*p==' ')
    {
      if (in_token)
      {
        count++;
        in_token=0;
      }
    }
    else in_token=1;
    p++;
  }
  if (in_token) count++;
  return count;
}

static void state_signature(const char *stackpos, const char *entrypoint,
                            const char *stack, char out[9])
{
  uint32_t hash=2166136261u;
  const char *parts[5]={stackpos," ",entrypoint," ",stack};
  int i;
  for (i=0;i<5;i++)
  {
    const unsigned char *p=(const unsigned char*)(parts[i] ? parts[i] : "");
    while (*p)
    {
      hash^=*p++;
      hash*=16777619u;
    }
  }
  snprintf(out,9,"%08X",hash);
}

static int run_evm_program_to_fd(int out_fd, const char *clean_path, int entrypoint, int stackpos, const char *stack)
{
  char full[8192];
  char *program;
  size_t program_len=0;
  int saved_stdout;
  int rc;

  snprintf(full,sizeof(full),"%s%s",g_www_root,clean_path);
  program=read_program_file(full,&program_len);
  if (!program) return 0;

  saved_stdout=dup(STDOUT_FILENO);
  if (saved_stdout<0 || dup2(out_fd,STDOUT_FILENO)<0)
  {
    if (saved_stdout>=0) close(saved_stdout);
    free(program);
    return 0;
  }

  rc=ewb_run_buffer(program,program_len,clean_path,entrypoint,stackpos,stack);
  fflush(stdout);
  dup2(saved_stdout,STDOUT_FILENO);
  close(saved_stdout);
  free(program);

  if (rc<0) warnx("VM returned %d for %s",rc,clean_path);
  return rc<0 ? -1 : 1;
}

static void handle_cron_task(const char *task, int generation, int *loaded_generation)
{
  ewb_cron_job job;
  int rc;
  int devnull;
  int entrypoint;
  int stackpos;
  const char *stack="";

  if (*loaded_generation!=generation)
  {
    *loaded_generation=generation;
  }

  rc=ewb_cron_load_job(task,&job);
  if (rc<=0) return;
  if (!job.program_url || !job.program_url[0] || !job.task || !job.task[0])
  {
    ewb_cron_job_free(&job);
    return;
  }

  entrypoint=atoi(job.task);
  if (job.stack) stack=job.stack;
  stackpos=count_stack_items(stack);
  devnull=open("/dev/null",O_WRONLY);
  if (devnull>=0)
  {
    run_evm_program_to_fd(devnull,job.program_url,entrypoint,stackpos,stack);
    close(devnull);
  }

  ewb_cron_job_free(&job);
}

// Esegue un programma .evm e manda l'output della VM sul socket HTTP.
static int serve_evm_program(int client_fd, const char *request_path, ewb_form *form)
{
  char clean[1024];
  char full[8192];
  char header[256];
  struct stat st;
  const char *stack="";
  int entrypoint=0;
  int stackpos=0;
  int rc;

  strip_query(request_path,clean,sizeof(clean));
  if (!is_evm_path(clean)) return 0;

  if (unsafe_path(clean))
  { http_error(client_fd,403,"Forbidden");
    return -1;
  }

  snprintf(full,sizeof(full),"%s%s",g_www_root,clean);
  if (stat(full,&st)<0 || !S_ISREG(st.st_mode))
  { http_error(client_fd,404,"Not Found");
    return -1;
  }

  if (form)
  { if (form->entrypoint) entrypoint=atoi(form->entrypoint);
    if (form->stackpos) stackpos=atoi(form->stackpos);
    if (form->stack) stack=form->stack;
    if (form->stack || form->entrypoint || form->stackpos)
    { char expected[9];
      state_signature(form->stackpos,form->entrypoint,stack,expected);
      if (!form->signature || strcmp(form->signature,expected))
      { http_error(client_fd,400,"Bad EWB signature");
        return -1;
      }
    }

    ewb_form_clear();
    for (int i=0;i<form->nfields;i++)
    { if (!strncmp(form->fields[i].name,"__",2)) continue;
      ewb_form_add_field(form->fields[i].name,form->fields[i].value,
                         form->fields[i].size);
    }
    for (int i=0;i<form->nfiles;i++)
      ewb_form_add_file(form->files[i].name,form->files[i].filename,
                        form->files[i].content_type,form->files[i].tmp_path,
                        form->files[i].size);
  }
  else ewb_form_clear();

  snprintf(header,sizeof(header),
           "HTTP/1.0 200 OK\r\n"
           "Content-Type: text/html; charset=utf-8\r\n"
           "Connection: close\r\n"
           "\r\n");
  write_all(client_fd,header);
  rc=run_evm_program_to_fd(client_fd,clean,entrypoint,stackpos,stack);
  return rc>0 ? 1 : -1;
}

static int path_inside_root(const char *path)
{
  size_t root_len=strlen(g_www_root);
  return !strncmp(path,g_www_root,root_len) &&
         (path[root_len]==0 || path[root_len]=='/');
}

static int serve_cgi_program(int client_fd, const char *request_path,
                             const char *method, const char *content_type,
                             const char *http_cookie,
                             const char *http_authorization,
                             const char *post_body, size_t post_body_len)
{
  char clean[1024];
  char joined[8192];
  char canonical[8192];
  char directory[8192];
  char length[64];
  const char *query=strchr(request_path,'?');
  struct stat st;
  FILE *input=NULL;
  int output_pipe[2]={-1,-1};
  pid_t child=-1;
  char *response=NULL;
  size_t used=0;
  int status=0;
  int timed_out=0;
  struct timespec started;

  strip_query(request_path,clean,sizeof(clean));
  if (!is_cgi_path(clean)) return 0;
  if (unsafe_path(clean))
  { http_error(client_fd,403,"Forbidden");
    return -1;
  }

  snprintf(joined,sizeof(joined),"%s%s",g_www_root,clean);
  if (!realpath(joined,canonical) || !path_inside_root(canonical) ||
      stat(canonical,&st)<0 || !S_ISREG(st.st_mode) ||
      !(st.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
  { http_error(client_fd,403,"Forbidden");
    return -1;
  }

  input=tmpfile();
  if (!input ||
      (post_body_len && fwrite(post_body,1,post_body_len,input)!=post_body_len) ||
      fflush(input)<0 || fseek(input,0,SEEK_SET)<0 ||
      set_cloexec(fileno(input))<0 || pipe(output_pipe)<0)
  { if (input) fclose(input);
    if (output_pipe[0]>=0) close(output_pipe[0]);
    if (output_pipe[1]>=0) close(output_pipe[1]);
    http_error(client_fd,502,"Bad Gateway");
    return -1;
  }

  child=fork();
  if (child==0)
  {
    char *slash;
    setpgid(0,0);
    dup2(fileno(input),STDIN_FILENO);
    dup2(output_pipe[1],STDOUT_FILENO);
    close(output_pipe[0]);
    close(output_pipe[1]);
    snprintf(directory,sizeof(directory),"%s",canonical);
    slash=strrchr(directory,'/');
    if (slash)
    { *slash=0;
      chdir(directory);
    }
    snprintf(length,sizeof(length),"%lu",(unsigned long)post_body_len);
    setenv("GATEWAY_INTERFACE","CGI/1.1",1);
    setenv("SERVER_PROTOCOL","HTTP/1.0",1);
    setenv("REQUEST_METHOD",method,1);
    setenv("SCRIPT_NAME",clean,1);
    setenv("SCRIPT_FILENAME",canonical,1);
    setenv("QUERY_STRING",query ? query+1 : "",1);
    setenv("CONTENT_TYPE",content_type ? content_type : "",1);
    setenv("CONTENT_LENGTH",length,1);
    setenv("HTTP_COOKIE",http_cookie ? http_cookie : "",1);
    setenv("HTTP_AUTHORIZATION",http_authorization ? http_authorization : "",1);
    execl(canonical,canonical,(char*)NULL);
    _exit(127);
  }

  fclose(input);
  close(output_pipe[1]);
  if (child<0)
  { close(output_pipe[0]);
    http_error(client_fd,502,"Bad Gateway");
    return -1;
  }
  setpgid(child,child);

  response=(char*)malloc(EWBD_CGI_MAX_RESPONSE+1);
  if (!response)
  { kill(child,SIGKILL);
    waitpid(child,NULL,0);
    close(output_pipe[0]);
    http_error(client_fd,502,"Bad Gateway");
    return -1;
  }

  clock_gettime(CLOCK_MONOTONIC,&started);
  for (;;)
  {
    fd_set rfds;
    struct timeval tv={0,100000};
    struct timespec now;
    ssize_t n;
    FD_ZERO(&rfds);
    FD_SET(output_pipe[0],&rfds);
    if (select(output_pipe[0]+1,&rfds,NULL,NULL,&tv)>0)
    {
      n=read(output_pipe[0],response+used,EWBD_CGI_MAX_RESPONSE-used+1);
      if (n>0)
      { used+=(size_t)n;
        if (used>EWBD_CGI_MAX_RESPONSE) break;
      }
      else if (n==0) break;
      else if (errno!=EINTR) break;
    }
    clock_gettime(CLOCK_MONOTONIC,&now);
    if (elapsed_ns(started,now) >= (uint64_t)g_cgi_timeout_sec*1000000000ULL)
    { timed_out=1;
      break;
    }
  }
  close(output_pipe[0]);
  while (!timed_out && used<=EWBD_CGI_MAX_RESPONSE)
  {
    pid_t waited=waitpid(child,&status,WNOHANG);
    struct timespec now;
    struct timespec pause={0,10000000};
    if (waited==child) break;
    if (waited<0)
    { timed_out=1;
      break;
    }
    clock_gettime(CLOCK_MONOTONIC,&now);
    if (elapsed_ns(started,now) >= (uint64_t)g_cgi_timeout_sec*1000000000ULL)
    { timed_out=1;
      break;
    }
    nanosleep(&pause,NULL);
  }
  if (timed_out || used>EWBD_CGI_MAX_RESPONSE)
  { kill(child,SIGKILL);
    kill(-child,SIGKILL);
    waitpid(child,NULL,0);
    free(response);
    http_error(client_fd,timed_out ? 504 : 502,
               timed_out ? "Gateway Timeout" : "Bad Gateway");
    return -1;
  }
  response[used]=0;

  char *separator=strstr(response,"\r\n\r\n");
  size_t separator_len=4;
  if (!separator)
  { separator=strstr(response,"\n\n");
    separator_len=2;
  }
  if (!separator || !WIFEXITED(status) || WEXITSTATUS(status)!=0)
  { free(response);
    http_error(client_fd,502,"Bad Gateway");
    return -1;
  }

  {
    int cgi_status=200;
    char cgi_reason[512];
    char status_line[1024];
    const char *line=response;
    if (parse_cgi_status(response,separator,&cgi_status,cgi_reason,
                         sizeof(cgi_reason))<0)
    { free(response);
      http_error(client_fd,502,"Bad Gateway");
      return -1;
    }
    snprintf(status_line,sizeof(status_line),"HTTP/1.0 %d %s\r\n",
             cgi_status,cgi_reason);
    write_all(client_fd,status_line);

    while (line<separator)
    {
      const char *next=memchr(line,'\n',(size_t)(separator-line));
      const char *line_end=next ? next : separator;
      size_t length=(size_t)(line_end-line);
      if (length>0 && line[length-1]=='\r') length--;
      if (!cgi_status_header(line,length))
      { write_all_n(client_fd,line,length);
        write_all(client_fd,"\r\n");
      }
      line=next ? next+1 : separator;
    }
  }
  write_all(client_fd,"Connection: close\r\n\r\n");
  {
    const char *body=separator+separator_len;
    size_t body_len=used-(size_t)(body-response);
    size_t done=0;
    while (done<body_len)
    {
      ssize_t n=write(client_fd,body+done,body_len-done);
      if (n<0)
      { if (errno==EINTR) continue;
        break;
      }
      done+=(size_t)n;
    }
  }
  free(response);
  return 1;
}

static int handle_client(int client_fd, int generation, int *loaded_generation,
                         int *is_evm)
{
  char req[EWBD_REQBUF];
  int req_len=0;
  char method[16];
  char path[1024];
  char indexed_path[1024];
  char resolved_path[1024];
  char body[4096];
  char header[512];
  char http_cookie[EWBD_REQBUF];
  char http_authorization[EWBD_REQBUF];
  ewb_form er;

  if (*loaded_generation!=generation)
  {
    /* Program cache flush placeholder. */
    *loaded_generation=generation;
  }

  memset(&er,0,sizeof(er));

  req_len=read_http_request(client_fd,req,sizeof(req));
  if (req_len<0) return 1;
  parse_request_line(req,method,sizeof(method),path,sizeof(path));
  header_value(req,"Cookie",http_cookie,sizeof(http_cookie));
  header_value(req,"Authorization",http_authorization,sizeof(http_authorization));

  if (method[0]==0) strcpy(method,"?");
  if (path[0]==0) strcpy(path,"/");
  default_index_path(path,indexed_path,sizeof(indexed_path));
  default_evm_path(indexed_path,resolved_path,sizeof(resolved_path));
  *is_evm=is_evm_path(resolved_path);

  if (!strcmp(method,"GET") || !strcmp(method,"HEAD"))
  { int served;
    if (!strcmp(method,"GET") && *is_evm)
      return serve_evm_program(client_fd,resolved_path,NULL)<0;
    if (!strcmp(method,"GET") && is_cgi_path(resolved_path))
      return serve_cgi_program(client_fd,resolved_path,method,"",http_cookie,
                               http_authorization,NULL,0)<0;
    served=serve_static_file(client_fd,resolved_path,!strcmp(method,"GET"));
    if (served) return served<0;
    http_error(client_fd,404,"Not Found");
    return 1;
  }

  if (!strcmp(method,"POST"))
  {
    char content_type[256];
    char *post_body=NULL;
    size_t post_body_len=0;
    content_type[0]=0;
    header_value(req,"Content-Type",content_type,sizeof(content_type));

    if (is_cgi_path(resolved_path))
    {
      if (read_http_body(client_fd,req,req_len,&post_body,&post_body_len)<0)
      { http_error(client_fd,400,"Bad POST body");
        return 1;
      }
      int failed=serve_cgi_program(client_fd,resolved_path,method,content_type,
                                   http_cookie,http_authorization,
                                   post_body,post_body_len)<0;
      free(post_body);
      return failed;
    }

    if (strncmp(content_type,"multipart/form-data",19) &&
        strncmp(content_type,"application/x-www-form-urlencoded",33))
    { http_error(client_fd,415,"Unsupported Media Type");
      return 1;
    }

    if (read_http_body(client_fd,req,req_len,&post_body,&post_body_len)<0)
    { http_error(client_fd,400,"Bad POST body");
      return 1;
    }

    if (ewb_form_parse(&er,content_type,post_body,post_body_len)<0)
    { http_error(client_fd,400,"Bad form body");
      ewb_form_free(&er);
      return 1;
    }

    if (*is_evm)
    { int failed=serve_evm_program(client_fd,resolved_path,&er)<0;
      ewb_form_free(&er);
      return failed;
    }

    const char *entrypoint="";
    const char *stackpos="";
    const char *signature="";
    unsigned long stack_size=0;
    if (er.entrypoint) entrypoint=er.entrypoint;
    if (er.stackpos) stackpos=er.stackpos;
    if (er.signature) signature=er.signature;
    if (er.stack) stack_size=(unsigned long)er.stack_size;

    char extra[2048];
    snprintf(extra,sizeof(extra),
             "<p>content-type: %s</p>"
             "<p>fields: %d</p>"
             "<p>files: %d</p>"
             "<p>__entrypoint: %s</p>"
             "<p>__stackpos: %s</p>"
             "<p>__stack bytes: %lu</p>"
             "<p>__signature: %s</p>",
             er.content_type,
             er.nfields,
             er.nfiles,
             entrypoint,
             stackpos,
             stack_size,
             signature);

    snprintf(body,sizeof(body),
             "<html><body>"
             "<h1>ewbd multipart request</h1>"
             "<p>pid: %ld</p>"
             "<p>generation: %d</p>"
             "<p>method: %s</p>"
             "<p>path: %s</p>"
             "%s"
             "</body></html>\n",
             (long)getpid(),generation,method,resolved_path,extra);

    snprintf(header,sizeof(header),
             "HTTP/1.0 200 OK\r\n"
             "Content-Type: text/html; charset=utf-8\r\n"
             "Content-Length: %lu\r\n"
             "Connection: close\r\n"
             "\r\n",
             (unsigned long)strlen(body));

    write_all(client_fd,header);
    write_all(client_fd,body);
    ewb_form_free(&er);
    return 0;
  }

  snprintf(body,sizeof(body),
           "<html><body>"
           "<h1>ewbd responder</h1>"
           "<p>pid: %ld</p>"
           "<p>generation: %d</p>"
           "<p>method: %s</p>"
           "<p>path: %s</p>"
           "</body></html>\n",
           (long)getpid(),generation,method,resolved_path);

  snprintf(header,sizeof(header),
           "HTTP/1.0 200 OK\r\n"
           "Content-Type: text/html; charset=utf-8\r\n"
           "Content-Length: %lu\r\n"
           "Connection: close\r\n"
           "\r\n",
           (unsigned long)strlen(body));

  write_all(client_fd,header);
  write_all(client_fd,body);
  return 0;
}

static void responder_loop(int control_fd)
{
  int loaded_generation=0;

  for (;;)
  {
    char tag=0;
    int client_fd=-1;
    int generation=0;
    char payload[EWBD_CTRL_PAYLOAD_MAX+1];
    int payload_len=0;

    if (recv_ctrl_msg(control_fd,&tag,&client_fd,&generation,payload,sizeof(payload),&payload_len)<0) exit(0);

    if (tag=='Q') exit(0);

    if (tag=='H')
    {
      loaded_generation=0;
      send_status(control_fd,'I');
      continue;
    }

    if (tag=='C')
    {
      handle_cron_task(payload,generation,&loaded_generation);
      send_status(control_fd,'I');
      continue;
    }

    if (tag!='R' || client_fd<0)
    {
      if (client_fd>=0) close(client_fd);
      send_status(control_fd,'I');
      continue;
    }

    if (set_cloexec(client_fd)<0)
    {
      http_error(client_fd,502,"Bad Gateway");
      close(client_fd);
      send_status(control_fd,'I');
      continue;
    }

    struct timespec started;
    struct timespec finished;
    struct timeval read_timeout;
    request_stat stat;
    memset(&stat,0,sizeof(stat));
    read_timeout.tv_sec=EWBD_CLIENT_READ_TIMEOUT_SEC;
    read_timeout.tv_usec=0;
    if (setsockopt(client_fd,SOL_SOCKET,SO_RCVTIMEO,
                   &read_timeout,sizeof(read_timeout))<0)
      warnx("cannot set client read timeout: %s",strerror(errno));
    clock_gettime(CLOCK_MONOTONIC,&started);
    stat.is_error=handle_client(client_fd,generation,&loaded_generation,&stat.is_evm);
    clock_gettime(CLOCK_MONOTONIC,&finished);
    stat.elapsed_ns=elapsed_ns(started,finished);
    close(client_fd);
    send_ctrl_msg(control_fd,'I',-1,0,(const char*)&stat,(int)sizeof(stat));
  }
}

static int spawn_responder(void)
{
  int sv[2];
  if (g_num_resp>=g_max_resp) return -1;
  if (socketpair(AF_UNIX,SOCK_SEQPACKET,0,sv)<0) return -1;

  pid_t pid=fork();
  if (pid<0)
  {
    close(sv[0]);
    close(sv[1]);
    return -1;
  }

  if (pid==0)
  {
    close(sv[0]);
    if (set_cloexec(sv[1])<0) _exit(1);
    responder_loop(sv[1]);
    exit(0);
  }

  close(sv[1]);
  set_cloexec(sv[0]);

  g_resp[g_num_resp].pid=pid;
  g_resp[g_num_resp].fd=sv[0];
  g_resp[g_num_resp].busy=0;
  g_resp[g_num_resp].generation=g_generation;
  g_num_resp++;

  return g_num_resp-1;
}

static void reap_children(void)
{
  int status;
  pid_t pid;

  while ((pid=waitpid(-1,&status,WNOHANG))>0)
  {
    for (int i=0; i<g_num_resp; i++)
    {
      if (g_resp[i].pid==pid)
      {
        close(g_resp[i].fd);
        g_resp[i]=g_resp[g_num_resp-1];
        g_num_resp--;
        break;
      }
    }
  }
}

static int create_listener(int port)
{
  int fd=socket(AF_INET,SOCK_STREAM,0);
  if (fd<0) die("socket failed");

  int yes=1;
  setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));

  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=htonl(INADDR_ANY);
  addr.sin_port=htons((unsigned short)port);

  if (bind(fd,(struct sockaddr*)&addr,sizeof(addr))<0) die("bind failed on port %d",port);
  if (listen(fd,128)<0) die("listen failed");
  return fd;
}

static int choose_responder(void)
{
  for (int i=0; i<g_num_resp; i++)
    if (!g_resp[i].busy && g_resp[i].generation==g_generation) return i;

  for (int i=0; i<g_num_resp; i++)
    if (!g_resp[i].busy) return i;

  return spawn_responder();
}

static void mark_hup(void)
{
  g_generation++;
  if (g_generation<=0) g_generation=1;

  for (int i=0; i<g_num_resp; i++)
  {
    g_resp[i].generation=0;
    if (!g_resp[i].busy)
    { send_ctrl_msg(g_resp[i].fd,'H',-1,g_generation,NULL,0);
      g_resp[i].generation=g_generation;
      g_resp[i].busy=1;
    }
  }
}

static void dispatch_cron_task(const char *task)
{
  int idx;
  int len;

  if (!task || !task[0]) return;
  idx=choose_responder();
  if (idx<0)
  {
    warnx("cron skipped, no responder available");
    return;
  }

  len=(int)strlen(task);
  g_resp[idx].busy=1;
  g_resp[idx].generation=g_generation;
  if (send_ctrl_msg(g_resp[idx].fd,'C',-1,g_generation,task,len)<0)
  {
    g_resp[idx].busy=0;
    warnx("cannot pass cron task to responder");
  }
}

static void cron_tick(void)
{
  static long last_minute=-1;
  ewb_cron_row *rows=NULL;
  int count=0;
  time_t now;
  long minute_now;
  struct tm tmv;

  now=time(NULL);
  minute_now=(long)(now/60);
  if (minute_now==last_minute) return;
  last_minute=minute_now;

  if (!localtime_r(&now,&tmv)) return;
  if (ewb_cron_list(&rows,&count)<0) return;

  for (int i=0; i<count; i++)
  {
    ewb_cron cron;
    char err[128];

    if (!rows[i].task || !rows[i].cronstring) continue;
    if (!cron_parse(rows[i].cronstring,&cron,err,sizeof(err))) continue;
    if (cron_match(&cron,&tmv)) dispatch_cron_task(rows[i].task);
  }

  ewb_cron_list_free(rows,count);
}

static void usage(const char *argv0)
{
  fprintf(stderr,
          "Usage: %s [-p port] [-m min_processes] [-M max_processes] "
          "[--www path] [--stats-file path] [--cgi-timeout seconds]\n",argv0);
  exit(1);
}

int main(int argc, char **argv)
{
  int port=8080;
  char canonical_root[sizeof(g_www_root)];
  struct stat root_stat;

  for (int i=1; i<argc; i++)
  {
    if (!strcmp(argv[i],"-p") && i+1<argc) port=atoi(argv[++i]);
    else if (!strcmp(argv[i],"-m") && i+1<argc) g_min_resp=atoi(argv[++i]);
    else if (!strcmp(argv[i],"-M") && i+1<argc) g_max_resp=atoi(argv[++i]);
    else if (!strcmp(argv[i],"--www") && i+1<argc)
    { strncpy(g_www_root,argv[++i],sizeof(g_www_root)-1);
      g_www_root[sizeof(g_www_root)-1]=0;
    }
    else if (!strcmp(argv[i],"--stats-file") && i+1<argc)
    { strncpy(g_stats_file,argv[++i],sizeof(g_stats_file)-1);
      g_stats_file[sizeof(g_stats_file)-1]=0;
    }
    else if (!strcmp(argv[i],"--cgi-timeout") && i+1<argc)
    {
      char *end=NULL;
      long seconds=strtol(argv[++i],&end,10);
      if (!end || *end || seconds<1 || seconds>86400) usage(argv[0]);
      g_cgi_timeout_sec=(int)seconds;
    }
    else usage(argv[0]);
  }

  if (!realpath(g_www_root,canonical_root) ||
      stat(canonical_root,&root_stat)<0 || !S_ISDIR(root_stat.st_mode))
    die("invalid www root: %s",g_www_root);
  snprintf(g_www_root,sizeof(g_www_root),"%s",canonical_root);

  if (g_min_resp<1) g_min_resp=1;
  if (g_max_resp<g_min_resp) g_max_resp=g_min_resp;
  if (g_max_resp>EWBD_MAX_RESPONDERS) g_max_resp=EWBD_MAX_RESPONDERS;

  signal(SIGHUP,on_hup);
  signal(SIGCHLD,on_chld);
  signal(SIGTERM,on_stop);
  signal(SIGINT,on_stop);
  signal(SIGPIPE,SIG_IGN);

  int listen_fd=create_listener(port);
  if (set_cloexec(listen_fd)<0) die("cannot protect listener from exec");

  for (int i=0; i<g_min_resp; i++)
    if (spawn_responder()<0) die("cannot spawn responder");

  g_stats_started=time(NULL);
  fprintf(stderr,
          "ewbd listening on port %d, responders min=%d max=%d, www=%s, stats=%s, cgi_timeout=%ds\n",
          port,g_min_resp,g_max_resp,g_www_root,g_stats_file,g_cgi_timeout_sec);

  while (!g_stop)
  {
    cron_tick();
    traffic_tick();

    if (g_hup)
    {
      g_hup=0;
      mark_hup();
      fprintf(stderr,"ewbd: SIGHUP, cache generation %d\n",g_generation);
    }

    if (g_chld)
    {
      g_chld=0;
      reap_children();
      while (g_num_resp<g_min_resp) spawn_responder();
    }

    fd_set rfds;
    FD_ZERO(&rfds);
    int maxfd=-1;
    int can_accept=0;

    for (int i=0; i<g_num_resp; i++)
      if (!g_resp[i].busy) can_accept=1;
    /* Grow before accept(): a responder forked afterwards would inherit the
       newly accepted client socket and keep the connection open. */
    if (!can_accept && g_num_resp<g_max_resp)
      if (spawn_responder()>=0) can_accept=1;

    if (can_accept)
    { FD_SET(listen_fd,&rfds);
      maxfd=listen_fd;
    }

    for (int i=0; i<g_num_resp; i++)
    {
      FD_SET(g_resp[i].fd,&rfds);
      if (g_resp[i].fd>maxfd) maxfd=g_resp[i].fd;
    }

    struct timeval tv;
    tv.tv_sec=1;
    tv.tv_usec=0;
    int rc=select(maxfd+1,&rfds,NULL,NULL,&tv);
    if (rc<0)
    {
      if (errno==EINTR) continue;
      die("select failed");
    }

    for (int i=0; i<g_num_resp; i++)
    {
      if (FD_ISSET(g_resp[i].fd,&rfds))
      {
        char tag=0;
        int fd=-1;
        int gen=0;
        char payload[EWBD_CTRL_PAYLOAD_MAX+1];
        int payload_len=0;
        if (recv_ctrl_msg(g_resp[i].fd,&tag,&fd,&gen,payload,sizeof(payload),&payload_len)<0)
        {
          close(g_resp[i].fd);
          kill(g_resp[i].pid,SIGTERM);
          g_resp[i]=g_resp[g_num_resp-1];
          g_num_resp--;
          i--;
          continue;
        }

        if (fd>=0) close(fd);
        if (tag=='I')
        {
          if (payload_len==(int)sizeof(request_stat))
          { request_stat stat;
            memcpy(&stat,payload,sizeof(stat));
            add_request_stat(&stat);
          }
          if (g_resp[i].generation!=g_generation)
          { send_ctrl_msg(g_resp[i].fd,'H',-1,g_generation,NULL,0);
            g_resp[i].generation=g_generation;
            g_resp[i].busy=1;
          }
          else
          { g_resp[i].busy=0;
          }
        }
      }
    }

    if (FD_ISSET(listen_fd,&rfds))
    {
      int client_fd=accept(listen_fd,NULL,NULL);
      if (client_fd<0)
      {
        if (errno!=EINTR) warnx("accept failed");
        continue;
      }

      int idx=choose_responder();
      if (idx<0)
      {
        request_stat stat;
        memset(&stat,0,sizeof(stat));
        stat.is_error=1;
        add_request_stat(&stat);
        write_all(client_fd,
          "HTTP/1.0 503 Service Unavailable\r\n"
          "Connection: close\r\n"
          "Content-Length: 12\r\n"
          "\r\n"
          "Busy server\n");
        close(client_fd);
        continue;
      }

      g_resp[idx].busy=1;
      g_resp[idx].generation=g_generation;
      if (send_ctrl_msg(g_resp[idx].fd,'R',client_fd,g_generation,NULL,0)<0)
      {
        request_stat stat;
        memset(&stat,0,sizeof(stat));
        stat.is_error=1;
        add_request_stat(&stat);
        g_resp[idx].busy=0;
        warnx("cannot pass client fd to responder");
      }
      close(client_fd);
    }
  }

  if (g_evm_stats.frames || g_other_stats.frames)
    if (write_traffic_counters(time(NULL))<0)
      warnx("cannot write final traffic counters to %s: %s",g_stats_file,strerror(errno));

  close(listen_fd);
  for (int i=0; i<g_num_resp; i++)
    send_ctrl_msg(g_resp[i].fd,'Q',-1,g_generation,NULL,0);
  for (int i=0; i<g_num_resp; i++)
  { waitpid(g_resp[i].pid,NULL,0);
    close(g_resp[i].fd);
  }
  return 0;
}
