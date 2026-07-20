#include "db_interface.h"
#include "cron_db.h"

//Database mysql
#define EWB_DB_WITH_MYSQL 1
//Database postgres
#define EWB_DB_WITH_POSTGRES 1
//Gestione manuale: cartelle con il nome del DB e files CSV con il nome della tabella 
#define EWB_DB_FILES 0


/*
 * Uso previsto dalla VM per il ciclo IN, senza cursori SQL vivi:
 *
 *   string qlist(string context, string query, string orderby);
 *   string qbyid(string context, string query, string orderby, string id);
 *
 *   code:
 *       push context
 *       push query
 *       push 0          'position
 *       QLIST
 *       push ids        'list of id
 *   loop:
 *       POP ids
 *       pop position
 *       ids[position]   'A=id
 *       jz end
 *       push context
 *       push query
 *       push id
 *       QBYID
 *       position++
 *       push position
 *       push ids
 *       record=A
 *       jz loop
 *       { corpo IN }
 *   end:
 *       { resto del programma }
 *
 * run_query(url, user, password, query, orderby):
 *   - select/with/show: ritorna il primo record
 *   - insert: ritorna l'insert id dove il backend lo consente
 *   - update/delete: ritorna il numero di righe modificate
 *
 * Le query devono essere deterministiche.
 * Se ORDER BY non è specificato viene aggiunto automaticamente ORDER BY id.
 * Se ORDER BY è specificato viene completato con ", id". 
 */

#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sstream>
#include <vector>

using namespace std;


#if EWB_DB_WITH_MYSQL
#include <mysql/mysql.h>
#endif

#if EWB_DB_WITH_POSTGRES
#include <postgresql/libpq-fe.h>
#endif

void raiseerr(string e);

struct DbUri
{ string engine;
  string database;
  unsigned int port;
};

static string checked_name(string name)
{ if (name=="") raiseerr("Empty DB name");
  for (size_t i=0; i<name.size(); i++)
  { unsigned char c=(unsigned char)name[i];
    if (!isalnum(c) && c!='_') raiseerr("Invalid DB name: "+name);
  }
  return name;
}

static string trim(string s)
{ size_t a=0;
  while (a<s.size() && isspace((unsigned char)s[a])) a++;

  size_t b=s.size();
  while (b>a && isspace((unsigned char)s[b-1])) b--;

  return s.substr(a,b-a);
}

static string lower_word(string s)
{ s=trim(s);
  string w;
  for (size_t i=0; i<s.size(); i++)
  { if (isspace((unsigned char)s[i])) break;
    w+=(char)tolower((unsigned char)s[i]);
  }
  return w;
}

static DbUri parse_context(string context)
{ DbUri u;
  u.engine="mysql";
  u.database="ewb";
  u.port=3306;

  size_t p=context.find("://");
  if (p==string::npos) raiseerr("DB context URI");

  u.engine=context.substr(0,p);
  string rest=context.substr(p+3);

  if (u.engine=="mysql") u.port=3306;
  else if (u.engine=="postgres" || u.engine=="postgresql")
  { u.engine="postgres";
    u.port=5432;
  }
  else raiseerr("DB engine");

  if (rest!="")
  { size_t c=rest.rfind(':');
    if (c==string::npos) u.database=rest;
    else
    { u.database=rest.substr(0,c);
      string ps=rest.substr(c+1);
      if (ps=="") raiseerr("DB context URI");
      u.port=(unsigned int)atoi(ps.c_str());
      if (!u.port) raiseerr("DB context URI");
    }
  }

  if (u.database=="") u.database="ewb";
  return u;
}

static string with_order(string q, string orderby)
{ orderby=trim(orderby);
  if (orderby=="") return q+" order by id";

  string low=lower_word(orderby);
  if (low=="order") return q+" "+orderby+", id";
  return q+" order by "+orderby+", id";
}

#if EWB_DB_WITH_MYSQL || EWB_DB_WITH_POSTGRES
static string env_or(const char *a, const char *b, const char *def)
{ const char *v=getenv(a);
  if (v && v[0]) return v;
  v=getenv(b);
  if (v && v[0]) return v;
  return def;
}

static string quote_value(string s)
{ string r="\"";
  for (char c: s)
  { if (c=='"') r+="\\\"";
    else if (c=='\\') r+="\\\\";
    else if (c=='\n') r+="\\n";
    else if (c=='\r') r+="\\r";
    else if (c=='\t') r+="\\t";
    else r+=c;
  }
  r+="\"";
  return r;
}

static string join_record(vector<string> row)
{ string r;
  for (size_t i=0; i<row.size(); i++)
  { if (i) r+=" ";
    r+=quote_value(row[i]);
  }
  return r;
}

static int has_where(string q)
{ string x=q;
  for (char &c: x) c=(char)tolower((unsigned char)c);
  return x.find(" where ")!=string::npos;
}

static string sql_string(string v)
{ string r="'";
  for (char c: v)
  { if (c=='\'') r+="''";
    else r+=c;
  }
  r+="'";
  return r;
}

string db_quote(string value)
{ return sql_string(value);
}

static string by_id_query(string query, string orderby, string id)
{ string sid=sql_string(id);
  size_t p=query.find("%ID%");
  while (p!=string::npos)
  { query.replace(p,4,sid);
    p=query.find("%ID%",p+sid.size());
  }

  if (query.find(sid)==string::npos)
  { if (has_where(query)) query += " and id=";
    else query += " where id=";
    query += sid;
  }

  return with_order(query,orderby);
}
#endif

#if EWB_DB_WITH_MYSQL
static string mysql_escape(MYSQL *db, string v)
{ string out;
  out.resize(v.size()*2+1);
  unsigned long n=mysql_real_escape_string(db,&out[0],v.c_str(),(unsigned long)v.size());
  out.resize(n);
  return "'"+out+"'";
}

static string mysql_error_string(MYSQL *db)
{ const char *e=mysql_error(db);
  if (!e || !e[0]) return "MySQL";
  return string("MySQL ")+e;
}

static MYSQL *mysql_open(DbUri u, string user, string pass)
{ if (user=="") user=env_or("EWB_MYSQL_USER","EWB_DB_USER","root");
  string host=env_or("EWB_MYSQL_HOST","EWB_DB_HOST","localhost");

  MYSQL *db=mysql_init(nullptr);
  if (!db) raiseerr("MySQL init");

  if (!mysql_real_connect(db,host.c_str(),user.c_str(),pass.c_str(),
                          u.database.c_str(),u.port,nullptr,0))
  { mysql_close(db);
    db=mysql_init(nullptr);
    if (!db) raiseerr("MySQL init");
    if (!mysql_real_connect(db,host.c_str(),user.c_str(),pass.c_str(),
                            nullptr,u.port,nullptr,0))
    { string e=mysql_error_string(db);
      mysql_close(db);
      raiseerr(e);
    }
    string database=checked_name(u.database);
    string sql="create database if not exists `"+database+"`";
    if (mysql_query(db,sql.c_str()) || mysql_select_db(db,database.c_str()))
    { string e=mysql_error_string(db);
      mysql_close(db);
      raiseerr(e);
    }
  }
  return db;
}

static void mysql_ensure_schema(MYSQL *db, string table, vector<string> fields)
{ table=checked_name(table);
  string qtable="`"+table+"`";
  string sql="create table if not exists "+qtable+
             " (`id` int unsigned not null auto_increment primary key)";
  if (mysql_query(db,sql.c_str())) raiseerr(mysql_error_string(db));
  sql="alter table "+qtable+
      " add column if not exists `id` int unsigned not null auto_increment primary key first";
  if (mysql_query(db,sql.c_str())) raiseerr(mysql_error_string(db));
  for (size_t i=0; i<fields.size(); i++)
  { string field=checked_name(fields[i]);
    if (field=="id") continue;
    sql="alter table "+qtable+" add column if not exists `"+field+"` text";
    if (mysql_query(db,sql.c_str())) raiseerr(mysql_error_string(db));
  }
}

static MYSQL *mysql_open_noerr(DbUri u)
{ string user=env_or("EWB_MYSQL_USER","EWB_DB_USER","root");
  string pass=env_or("EWB_MYSQL_PASS","EWB_DB_PASS","");
  string host=env_or("EWB_MYSQL_HOST","EWB_DB_HOST","localhost");

  MYSQL *db=mysql_init(nullptr);
  if (!db) return NULL;

  if (!mysql_real_connect(db,host.c_str(),user.c_str(),pass.c_str(),
                          u.database.c_str(),u.port,nullptr,0))
  { mysql_close(db);
    return NULL;
  }
  return db;
}

static MYSQL *mysql_open_bootstrap(DbUri u)
{ string user=env_or("EWB_MYSQL_USER","EWB_DB_USER","root");
  string pass=env_or("EWB_MYSQL_PASS","EWB_DB_PASS","");
  string host=env_or("EWB_MYSQL_HOST","EWB_DB_HOST","localhost");

  MYSQL *db=mysql_init(nullptr);
  if (!db) raiseerr("MySQL init");

  if (!mysql_real_connect(db,host.c_str(),user.c_str(),pass.c_str(),
                          nullptr,u.port,nullptr,0))
  { string e=mysql_error_string(db);
    mysql_close(db);
    raiseerr(e);
  }
  return db;
}

static vector<string> mysql_first_row(MYSQL *db)
{ MYSQL_RES *res=mysql_store_result(db);
  vector<string> out;
  if (!res) return out;

  MYSQL_ROW row=mysql_fetch_row(res);
  if (!row)
  { mysql_free_result(res);
    return out;
  }

  unsigned int n=mysql_num_fields(res);
  unsigned long *len=mysql_fetch_lengths(res);
  for (unsigned int i=0; i<n; i++)
  { if (row[i]) out.push_back(string(row[i],len[i]));
    else out.push_back("");
  }

  mysql_free_result(res);
  return out;
}

static string mysql_first_record(MYSQL *db)
{ return join_record(mysql_first_row(db));
}

static string mysql_list_ids(MYSQL *db)
{ MYSQL_RES *res=mysql_store_result(db);
  if (!res) return "";

  string out;
  MYSQL_ROW row;
  unsigned long *len;
  while ((row=mysql_fetch_row(res)))
  { len=mysql_fetch_lengths(res);
    if (out!="") out+="\n";
    if (row[0]) out+=string(row[0],len[0]);
    else out+="";
  }

  mysql_free_result(res);
  return out;
}
#endif

void create_base_database(void)
{ DbUri u=parse_context(DEFAULTENGINE);

  if (u.engine=="mysql")
  {
#if EWB_DB_WITH_MYSQL
    MYSQL *db=mysql_open_bootstrap(u);
    string sql="create database if not exists `"+u.database+"`";
    if (mysql_query(db,sql.c_str()))
    { string e=mysql_error_string(db);
      mysql_close(db);
      raiseerr(e);
    }
    mysql_close(db);
    return;
#else
    raiseerr("MySQL support not compiled");
#endif
  }

  if (u.engine=="postgres")
  { raiseerr("Postgres bootstrap not implemented");
  }

  raiseerr("DB engine");
}

static char *dup_field(const char *s, unsigned long len)
{ char *out;
  out=(char*)malloc((size_t)len+1);
  if (!out) return NULL;
  memcpy(out,s,len);
  out[len]=0;
  return out;
}

extern "C" int ewb_cron_list(ewb_cron_row **rows_out, int *count_out)
{ DbUri u=parse_context(DEFAULTENGINE);
  MYSQL *db;
  MYSQL_RES *res;
  MYSQL_ROW row;
  unsigned long *len;
  ewb_cron_row *rows;
  int count;
  int i;

  if (!rows_out || !count_out) return -1;
  *rows_out=NULL;
  *count_out=0;
  if (u.engine!="mysql") return -1;
#if !EWB_DB_WITH_MYSQL
  return -1;
#else
  db=mysql_open_noerr(u);
  if (!db) return -1;
  if (mysql_query(db,"select task, cronstring from _crontab order by task"))
  { mysql_close(db);
    return -1;
  }
  res=mysql_store_result(db);
  if (!res)
  { mysql_close(db);
    return -1;
  }
  count=(int)mysql_num_rows(res);
  if (count<=0)
  { mysql_free_result(res);
    mysql_close(db);
    return 0;
  }
  rows=(ewb_cron_row*)calloc((size_t)count,sizeof(ewb_cron_row));
  if (!rows)
  { mysql_free_result(res);
    mysql_close(db);
    return -1;
  }
  i=0;
  while ((row=mysql_fetch_row(res)))
  { len=mysql_fetch_lengths(res);
    if (row[0]) rows[i].task=dup_field(row[0],len[0]);
    if (row[1]) rows[i].cronstring=dup_field(row[1],len[1]);
    i++;
  }
  mysql_free_result(res);
  mysql_close(db);
  *rows_out=rows;
  *count_out=i;
  return 0;
#endif
}

extern "C" void ewb_cron_list_free(ewb_cron_row *rows, int count)
{ int i;
  if (!rows) return;
  for (i=0; i<count; i++)
  { free(rows[i].task);
    free(rows[i].cronstring);
  }
  free(rows);
}

extern "C" int ewb_cron_load_job(const char *task, ewb_cron_job *job)
{ DbUri u=parse_context(DEFAULTENGINE);
  MYSQL *db;
  MYSQL_RES *res;
  MYSQL_ROW row;
  unsigned long *len;
  string sql;
  string qtask;

  if (!task || !job) return -1;
  memset(job,0,sizeof(*job));
  if (u.engine!="mysql") return -1;
#if !EWB_DB_WITH_MYSQL
  return -1;
#else
  db=mysql_open_noerr(u);
  if (!db) return -1;
  qtask=mysql_escape(db,task);
  sql="select task, program_url, stack, parameters, cronstring from _crontab where task="+qtask;
  if (mysql_query(db,sql.c_str()))
  { mysql_close(db);
    return -1;
  }
  res=mysql_store_result(db);
  if (!res)
  { mysql_close(db);
    return -1;
  }
  row=mysql_fetch_row(res);
  if (!row)
  { mysql_free_result(res);
    mysql_close(db);
    return 0;
  }
  len=mysql_fetch_lengths(res);
  if (row[0]) job->task=dup_field(row[0],len[0]);
  if (row[1]) job->program_url=dup_field(row[1],len[1]);
  if (row[2]) job->stack=dup_field(row[2],len[2]);
  if (row[3]) job->parameters=dup_field(row[3],len[3]);
  if (row[4]) job->cronstring=dup_field(row[4],len[4]);
  mysql_free_result(res);
  mysql_close(db);
  return 1;
#endif
}

extern "C" void ewb_cron_job_free(ewb_cron_job *job)
{ if (!job) return;
  free(job->task);
  free(job->program_url);
  free(job->stack);
  free(job->parameters);
  free(job->cronstring);
  memset(job,0,sizeof(*job));
}

#if EWB_DB_WITH_POSTGRES
static PGconn *pg_open(DbUri u, string user, string pass)
{ if (user=="") user=env_or("EWB_PG_USER","EWB_DB_USER","");
  string host=env_or("EWB_PG_HOST","EWB_DB_HOST","localhost");

  ostringstream ss;
  ss << "host=" << host << " dbname=" << u.database << " port=" << u.port;
  if (user!="") ss << " user=" << user;
  if (pass!="") ss << " password=" << pass;

  PGconn *db=PQconnectdb(ss.str().c_str());
  if (PQstatus(db)!=CONNECTION_OK)
  { PQfinish(db);
    ostringstream admin;
    admin << "host=" << host << " dbname=postgres port=" << u.port;
    if (user!="") admin << " user=" << user;
    if (pass!="") admin << " password=" << pass;
    db=PQconnectdb(admin.str().c_str());
    if (PQstatus(db)!=CONNECTION_OK)
    { string e=PQerrorMessage(db);
      PQfinish(db);
      raiseerr("Postgres "+e);
    }
    string database=checked_name(u.database);
    PGresult *res=PQexec(db,("create database \""+database+"\"").c_str());
    if (PQresultStatus(res)!=PGRES_COMMAND_OK)
    { const char *state=PQresultErrorField(res,PG_DIAG_SQLSTATE);
      bool exists=false;
      if (state && string(state)=="42P04") exists=true;
      if (!exists)
      { string e=PQerrorMessage(db);
        PQclear(res);
        PQfinish(db);
        raiseerr("Postgres "+e);
      }
    }
    PQclear(res);
    PQfinish(db);
    db=PQconnectdb(ss.str().c_str());
    if (PQstatus(db)!=CONNECTION_OK)
    { string e=PQerrorMessage(db);
      PQfinish(db);
      raiseerr("Postgres "+e);
    }
  }
  return db;
}

static void pg_ensure_schema(PGconn *db, string table, vector<string> fields)
{ table=checked_name(table);
  string qtable="\""+table+"\"";
  string sql="create table if not exists "+qtable+
             " (\"id\" serial primary key)";
  PGresult *res=PQexec(db,sql.c_str());
  if (PQresultStatus(res)!=PGRES_COMMAND_OK)
  { string e=PQerrorMessage(db); PQclear(res); raiseerr("Postgres "+e); }
  PQclear(res);
  sql="alter table "+qtable+
      " add column if not exists \"id\" serial primary key";
  res=PQexec(db,sql.c_str());
  if (PQresultStatus(res)!=PGRES_COMMAND_OK)
  { string e=PQerrorMessage(db); PQclear(res); raiseerr("Postgres "+e); }
  PQclear(res);
  for (size_t i=0; i<fields.size(); i++)
  { string field=checked_name(fields[i]);
    if (field=="id") continue;
    sql="alter table "+qtable+" add column if not exists \""+field+"\" text";
    res=PQexec(db,sql.c_str());
    if (PQresultStatus(res)!=PGRES_COMMAND_OK)
    { string e=PQerrorMessage(db); PQclear(res); raiseerr("Postgres "+e); }
    PQclear(res);
  }
}

static string pg_escape(PGconn *db, string v)
{ int err=0;
  char *q=PQescapeLiteral(db,v.c_str(),v.size());
  if (!q || err) raiseerr("Postgres escape");
  string r=q;
  PQfreemem(q);
  return r;
}

static vector<string> pg_first_row(PGresult *res)
{ if (PQntuples(res)<1) return vector<string>();

  vector<string> out;
  for (int i=0; i<PQnfields(res); i++)
  { if (PQgetisnull(res,0,i)) out.push_back("");
    else out.push_back(PQgetvalue(res,0,i));
  }
  return out;
}

static string pg_first_record(PGresult *res)
{ return join_record(pg_first_row(res));
}

static string pg_list_ids(PGresult *res)
{ string out;
  for (int i=0; i<PQntuples(res); i++)
  { if (i) out+="\n";
    if (!PQgetisnull(res,i,0)) out+=PQgetvalue(res,i,0);
  }
  return out;
}
#endif

string qlist(string url, string user, string password, string table,
             vector<string> fields, string filter, string orderby)
{ DbUri u=parse_context(url);
  string sql="select id from "+table;
  filter=trim(filter);
  if (filter!="" && filter!="true") sql+=" where "+filter;
  if (orderby=="") { orderby="id"; } else orderby+=",id"; 
  sql=with_order(sql,orderby);
  (void)sql;

  if (u.engine=="mysql")
  {
#if EWB_DB_WITH_MYSQL
    MYSQL *db=mysql_open(u,user,password);
    mysql_ensure_schema(db,table,fields);
    if (mysql_query(db,sql.c_str()))
    { string e=mysql_error_string(db);
      mysql_close(db);
      raiseerr(e);
    }
    string r=mysql_list_ids(db);
    mysql_close(db);
    return r;
#else
    raiseerr("MySQL support not compiled");
#endif
  }

  if (u.engine=="postgres")
  {
#if EWB_DB_WITH_POSTGRES
    PGconn *db=pg_open(u,user,password);
    pg_ensure_schema(db,table,fields);
    PGresult *res=PQexec(db,sql.c_str());
    if (PQresultStatus(res)!=PGRES_TUPLES_OK)
    { string e=PQerrorMessage(db);
      PQclear(res);
      PQfinish(db);
      raiseerr("Postgres "+e);
    }
    string r=pg_list_ids(res);
    PQclear(res);
    PQfinish(db);
    return r;
#else
    raiseerr("Postgres support not compiled");
#endif
  }

  raiseerr("DB engine");
  return "";
}

vector<string> qbyid(string url, string user, string password, string table,
                     vector<string> fields, string orderby,
                     string id)
{ DbUri u=parse_context(url);
  string query="select ";
  for (size_t i=0; i<fields.size(); i++)
  { if (i>0) query+=",";
    query+=fields[i];
  }
  query+=" from "+table + " where id=" + id + " ";
  if (u.engine=="mysql")
  {
#if EWB_DB_WITH_MYSQL
    MYSQL *db=mysql_open(u,user,password);
    mysql_ensure_schema(db,table,fields);
    string sql=by_id_query(query,orderby,id);
    size_t p=sql.find(sql_string(id));
    if (p!=string::npos) sql.replace(p,sql_string(id).size(),mysql_escape(db,id));
    if (mysql_query(db,sql.c_str()))
    { string e=mysql_error_string(db);
      mysql_close(db);
      raiseerr(e);
    }
    vector<string> r=mysql_first_row(db);
    mysql_close(db);
    return r;
#else
    raiseerr("MySQL support not compiled");
#endif
  }

  if (u.engine=="postgres")
  {
#if EWB_DB_WITH_POSTGRES
    PGconn *db=pg_open(u,user,password);
    pg_ensure_schema(db,table,fields);
    string sql=by_id_query(query,orderby,id);
    size_t p=sql.find(sql_string(id));
    if (p!=string::npos) sql.replace(p,sql_string(id).size(),pg_escape(db,id));
    PGresult *res=PQexec(db,sql.c_str());
    if (PQresultStatus(res)!=PGRES_TUPLES_OK)
    { string e=PQerrorMessage(db);
      PQclear(res);
      PQfinish(db);
      raiseerr("Postgres "+e);
    }
    vector<string> r=pg_first_row(res);
    PQclear(res);
    PQfinish(db);
    return r;
#else
    raiseerr("Postgres support not compiled");
#endif
  }

  raiseerr("DB engine");
  return vector<string>();
}

string run_query(string url, string user, string password, string query,
                 string orderby)
{ DbUri u=parse_context(url);
  string op=lower_word(query);
  string sql=query;
  if (op=="select" || op=="with" || op=="show") sql=with_order(query,orderby);

  if (u.engine=="mysql")
  {
#if EWB_DB_WITH_MYSQL
    MYSQL *db=mysql_open(u,user,password);
    if (mysql_query(db,sql.c_str()))
    { string e=mysql_error_string(db);
      mysql_close(db);
      raiseerr(e);
    }
    string r;
    if (op=="select" || op=="with" || op=="show") r=mysql_first_record(db);
    else if (op=="insert") r=to_string((unsigned long long)mysql_insert_id(db));
    else r=to_string((unsigned long long)mysql_affected_rows(db));
    mysql_close(db);
    return r;
#else
    raiseerr("MySQL support not compiled");
#endif
  }

  if (u.engine=="postgres")
  {
#if EWB_DB_WITH_POSTGRES
    PGconn *db=pg_open(u,user,password);
    PGresult *res=PQexec(db,sql.c_str());
    ExecStatusType st=PQresultStatus(res);
    string r;
    if (st==PGRES_TUPLES_OK) r=pg_first_record(res);
    else if (st==PGRES_COMMAND_OK)
    { char *n=PQcmdTuples(res);
      if (n && n[0]) r=n;
      else r="0";
      if (op=="insert")
      { Oid oid=PQoidValue(res);
        if (oid) r=to_string((unsigned long)oid);
      }
    }
    else
    { string e=PQerrorMessage(db);
      PQclear(res);
      PQfinish(db);
      raiseerr("Postgres "+e);
    }
    PQclear(res);
    PQfinish(db);
    return r;
#else
    raiseerr("Postgres support not compiled");
#endif
  }

  raiseerr("DB engine");
  return "";
}

string run_query(string url, string user, string password, string table,
                 vector<string> fields, string query, string orderby)
{ DbUri u=parse_context(url);
  if (u.engine=="mysql")
  {
#if EWB_DB_WITH_MYSQL
    MYSQL *db=mysql_open(u,user,password);
    mysql_ensure_schema(db,table,fields);
    mysql_close(db);
#endif
  }
  if (u.engine=="postgres")
  {
#if EWB_DB_WITH_POSTGRES
    PGconn *db=pg_open(u,user,password);
    pg_ensure_schema(db,table,fields);
    PQfinish(db);
#endif
  }
  return run_query(url,user,password,query,orderby);
}
