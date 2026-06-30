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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define EWBD_MAX_RESPONDERS 1024
#define EWBD_REQBUF 16384

typedef struct responder
{
  pid_t pid;
  int fd;
  int busy;
  int generation;
} responder;

static volatile sig_atomic_t g_hup=0;
static volatile sig_atomic_t g_chld=0;
static responder g_resp[EWBD_MAX_RESPONDERS];
static int g_num_resp=0;
static int g_min_resp=2;
static int g_max_resp=16;
static int g_generation=1;
static char g_www_root[4096]=".";

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

static int set_cloexec(int fd)
{
  int flags=fcntl(fd,F_GETFD,0);
  if (flags<0) return -1;
  return fcntl(fd,F_SETFD,flags|FD_CLOEXEC);
}

static int send_fd_msg(int sock, char tag, int fd_to_send, int generation)
{
  char data[1 + sizeof(int)];
  struct iovec iov;
  struct msghdr msg;
  char control[CMSG_SPACE(sizeof(int))];

  data[0]=tag;
  memcpy(data+1,&generation,sizeof(int));

  memset(&msg,0,sizeof(msg));
  iov.iov_base=data;
  iov.iov_len=sizeof(data);
  msg.msg_iov=&iov;
  msg.msg_iovlen=1;

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

  if (sendmsg(sock,&msg,0)!=(ssize_t)sizeof(data)) return -1;
  return 0;
}

static int recv_fd_msg(int sock, char *tag, int *fd_out, int *generation)
{
  char data[1 + sizeof(int)];
  struct iovec iov;
  struct msghdr msg;
  char control[CMSG_SPACE(sizeof(int))];

  *fd_out=-1;
  *generation=0;

  memset(&msg,0,sizeof(msg));
  memset(control,0,sizeof(control));
  iov.iov_base=data;
  iov.iov_len=sizeof(data);
  msg.msg_iov=&iov;
  msg.msg_iovlen=1;
  msg.msg_control=control;
  msg.msg_controllen=sizeof(control);

  ssize_t n=recvmsg(sock,&msg,0);
  if (n<=0) return -1;
  if (n<(ssize_t)sizeof(data)) return -1;

  *tag=data[0];
  memcpy(generation,data+1,sizeof(int));

  struct cmsghdr *cmsg=CMSG_FIRSTHDR(&msg);
  if (cmsg && cmsg->cmsg_level==SOL_SOCKET && cmsg->cmsg_type==SCM_RIGHTS)
    memcpy(fd_out,CMSG_DATA(cmsg),sizeof(int));

  return 0;
}

static int send_status(int sock, char tag)
{
  int gen=0;
  return send_fd_msg(sock,tag,-1,gen);
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

static void parse_request_line(const char *req, char *method, size_t msz, char *path, size_t psz)
{
  method[0]=0;
  path[0]=0;
  sscanf(req,"%15s %1023s",method,path);
  method[msz-1]=0;
  path[psz-1]=0;
}

static void write_all(int fd, const char *s)
{
  size_t left=strlen(s);
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

static const char *mime_type(const char *path)
{
  const char *ext=strrchr(path,'.');
  if (!ext) return "application/octet-stream";
  if (!strcmp(ext,".html") || !strcmp(ext,".htm")) return "text/html; charset=utf-8";
  if (!strcmp(ext,".css")) return "text/css; charset=utf-8";
  if (!strcmp(ext,".js")) return "application/javascript; charset=utf-8";
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
         !strcmp(ext,".png")  || !strcmp(ext,".jpg") ||
         !strcmp(ext,".jpeg") || !strcmp(ext,".gif") ||
         !strcmp(ext,".webp") || !strcmp(ext,".svg") ||
         !strcmp(ext,".ico")  || !strcmp(ext,".txt");
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

static int serve_static_file(int client_fd, const char *request_path)
{
  char clean[1024];
  char full[8192];
  char header[512];
  struct stat st;

  strip_query(request_path,clean,sizeof(clean));
  if (!is_static_path(clean)) return 0;

  if (unsafe_path(clean))
  { http_error(client_fd,403,"Forbidden");
    return 1;
  }

  snprintf(full,sizeof(full),"%s%s",g_www_root,clean);

  if (stat(full,&st)<0 || !S_ISREG(st.st_mode))
  { http_error(client_fd,404,"Not Found");
    return 1;
  }

  int f=open(full,O_RDONLY);
  if (f<0)
  { http_error(client_fd,403,"Forbidden");
    return 1;
  }

  snprintf(header,sizeof(header),
           "HTTP/1.0 200 OK\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %lu\r\n"
           "Connection: close\r\n"
           "\r\n",
           mime_type(clean),(unsigned long)st.st_size);
  write_all(client_fd,header);

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
        return 1;
      }
      done+=w;
    }
  }
  close(f);
  return 1;
}

static void handle_client(int client_fd, int generation, int *loaded_generation)
{
  char req[EWBD_REQBUF];
  char method[16];
  char path[1024];
  char body[4096];
  char header[512];

  if (*loaded_generation!=generation)
  {
    /* Program cache flush placeholder. */
    *loaded_generation=generation;
  }

  if (read_http_request(client_fd,req,sizeof(req))<0) return;
  parse_request_line(req,method,sizeof(method),path,sizeof(path));

  if (method[0]==0) strcpy(method,"?");
  if (path[0]==0) strcpy(path,"/");

  if ((!strcmp(method,"GET") || !strcmp(method,"HEAD")) && serve_static_file(client_fd,path))
    return;

  snprintf(body,sizeof(body),
           "<html><body>"
           "<h1>ewbd responder</h1>"
           "<p>pid: %ld</p>"
           "<p>generation: %d</p>"
           "<p>method: %s</p>"
           "<p>path: %s</p>"
           "</body></html>\n",
           (long)getpid(),generation,method,path);

  snprintf(header,sizeof(header),
           "HTTP/1.0 200 OK\r\n"
           "Content-Type: text/html; charset=utf-8\r\n"
           "Content-Length: %lu\r\n"
           "Connection: close\r\n"
           "\r\n",
           (unsigned long)strlen(body));

  write_all(client_fd,header);
  write_all(client_fd,body);
}

static void responder_loop(int control_fd)
{
  int loaded_generation=0;

  for (;;)
  {
    char tag=0;
    int client_fd=-1;
    int generation=0;

    if (recv_fd_msg(control_fd,&tag,&client_fd,&generation)<0) exit(0);

    if (tag=='Q') exit(0);

    if (tag=='H')
    {
      loaded_generation=0;
      send_status(control_fd,'I');
      continue;
    }

    if (tag!='R' || client_fd<0)
    {
      if (client_fd>=0) close(client_fd);
      send_status(control_fd,'I');
      continue;
    }

    handle_client(client_fd,generation,&loaded_generation);
    close(client_fd);
    send_status(control_fd,'I');
  }
}

static int spawn_responder(void)
{
  int sv[2];
  if (g_num_resp>=g_max_resp) return -1;
  if (socketpair(AF_UNIX,SOCK_STREAM,0,sv)<0) return -1;

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
    { send_fd_msg(g_resp[i].fd,'H',-1,g_generation);
      g_resp[i].generation=g_generation;
      g_resp[i].busy=1;
    }
  }
}

static void usage(const char *argv0)
{
  fprintf(stderr,"Usage: %s [-p port] [-m min_processes] [-M max_processes] [--www path]\n",argv0);
  exit(1);
}

int main(int argc, char **argv)
{
  int port=8080;

  for (int i=1; i<argc; i++)
  {
    if (!strcmp(argv[i],"-p") && i+1<argc) port=atoi(argv[++i]);
    else if (!strcmp(argv[i],"-m") && i+1<argc) g_min_resp=atoi(argv[++i]);
    else if (!strcmp(argv[i],"-M") && i+1<argc) g_max_resp=atoi(argv[++i]);
    else if (!strcmp(argv[i],"--www") && i+1<argc)
    { strncpy(g_www_root,argv[++i],sizeof(g_www_root)-1);
      g_www_root[sizeof(g_www_root)-1]=0;
    }
    else usage(argv[0]);
  }

  if (!strcmp(g_www_root,"."))
  { if (!getcwd(g_www_root,sizeof(g_www_root))) strcpy(g_www_root,".");
  }

  if (g_min_resp<1) g_min_resp=1;
  if (g_max_resp<g_min_resp) g_max_resp=g_min_resp;
  if (g_max_resp>EWBD_MAX_RESPONDERS) g_max_resp=EWBD_MAX_RESPONDERS;

  signal(SIGHUP,on_hup);
  signal(SIGCHLD,on_chld);
  signal(SIGPIPE,SIG_IGN);

  int listen_fd=create_listener(port);

  for (int i=0; i<g_min_resp; i++)
    if (spawn_responder()<0) die("cannot spawn responder");

  fprintf(stderr,"ewbd listening on port %d, responders min=%d max=%d, www=%s\n",
          port,g_min_resp,g_max_resp,g_www_root);

  for (;;)
  {
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
    FD_SET(listen_fd,&rfds);
    int maxfd=listen_fd;

    for (int i=0; i<g_num_resp; i++)
    {
      FD_SET(g_resp[i].fd,&rfds);
      if (g_resp[i].fd>maxfd) maxfd=g_resp[i].fd;
    }

    int rc=select(maxfd+1,&rfds,NULL,NULL,NULL);
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
        if (recv_fd_msg(g_resp[i].fd,&tag,&fd,&gen)<0)
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
          if (g_resp[i].generation!=g_generation)
          { send_fd_msg(g_resp[i].fd,'H',-1,g_generation);
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
      if (send_fd_msg(g_resp[idx].fd,'R',client_fd,g_generation)<0)
      {
        g_resp[idx].busy=0;
        warnx("cannot pass client fd to responder");
      }
      close(client_fd);
    }
  }
}
