#include "db_interface.h"

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
 *       push order
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
 *       push order
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
 * run_query(context, query, orderby):
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
  { query += has_where(query) ? " and id=" : " where id=";
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

static MYSQL *mysql_open(DbUri u)
{ string user=env_or("EWB_MYSQL_USER","EWB_DB_USER","root");
  string pass=env_or("EWB_MYSQL_PASS","EWB_DB_PASS","");
  string host=env_or("EWB_MYSQL_HOST","EWB_DB_HOST","localhost");

  MYSQL *db=mysql_init(nullptr);
  if (!db) raiseerr("MySQL init");

  if (!mysql_real_connect(db,host.c_str(),user.c_str(),pass.c_str(),
                          u.database.c_str(),u.port,nullptr,0))
  { string e=mysql_error_string(db);
    mysql_close(db);
    raiseerr(e);
  }
  return db;
}

static string mysql_first_record(MYSQL *db)
{ MYSQL_RES *res=mysql_store_result(db);
  if (!res) return "";

  MYSQL_ROW row=mysql_fetch_row(res);
  if (!row)
  { mysql_free_result(res);
    return "";
  }

  unsigned int n=mysql_num_fields(res);
  unsigned long *len=mysql_fetch_lengths(res);
  vector<string> out;
  for (unsigned int i=0; i<n; i++)
    out.push_back(row[i] ? string(row[i],len[i]) : "");

  mysql_free_result(res);
  return join_record(out);
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
    out+=row[0] ? string(row[0],len[0]) : "";
  }

  mysql_free_result(res);
  return out;
}
#endif

#if EWB_DB_WITH_POSTGRES
static PGconn *pg_open(DbUri u)
{ string user=env_or("EWB_PG_USER","EWB_DB_USER","");
  string pass=env_or("EWB_PG_PASS","EWB_DB_PASS","");
  string host=env_or("EWB_PG_HOST","EWB_DB_HOST","localhost");

  ostringstream ss;
  ss << "host=" << host << " dbname=" << u.database << " port=" << u.port;
  if (user!="") ss << " user=" << user;
  if (pass!="") ss << " password=" << pass;

  PGconn *db=PQconnectdb(ss.str().c_str());
  if (PQstatus(db)!=CONNECTION_OK)
  { string e=PQerrorMessage(db);
    PQfinish(db);
    raiseerr("Postgres "+e);
  }
  return db;
}

static string pg_escape(PGconn *db, string v)
{ int err=0;
  char *q=PQescapeLiteral(db,v.c_str(),v.size());
  if (!q || err) raiseerr("Postgres escape");
  string r=q;
  PQfreemem(q);
  return r;
}

static string pg_first_record(PGresult *res)
{ if (PQntuples(res)<1) return "";

  vector<string> out;
  for (int i=0; i<PQnfields(res); i++)
    out.push_back(PQgetisnull(res,0,i) ? "" : PQgetvalue(res,0,i));
  return join_record(out);
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

string qlist(string context, string query, string orderby)
{ DbUri u=parse_context(context);
  string sql=with_order(query,orderby);
  (void)sql;

  if (u.engine=="mysql")
  {
#if EWB_DB_WITH_MYSQL
    MYSQL *db=mysql_open(u);
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
    PGconn *db=pg_open(u);
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

string qbyid(string context, string query, string orderby, string id)
{ DbUri u=parse_context(context);
  (void)query;
  (void)orderby;
  (void)id;

  if (u.engine=="mysql")
  {
#if EWB_DB_WITH_MYSQL
    MYSQL *db=mysql_open(u);
    string sql=by_id_query(query,orderby,id);
    size_t p=sql.find(sql_string(id));
    if (p!=string::npos) sql.replace(p,sql_string(id).size(),mysql_escape(db,id));
    if (mysql_query(db,sql.c_str()))
    { string e=mysql_error_string(db);
      mysql_close(db);
      raiseerr(e);
    }
    string r=mysql_first_record(db);
    mysql_close(db);
    return r;
#else
    raiseerr("MySQL support not compiled");
#endif
  }

  if (u.engine=="postgres")
  {
#if EWB_DB_WITH_POSTGRES
    PGconn *db=pg_open(u);
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
    string r=pg_first_record(res);
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

string run_query(string context, string query, string orderby)
{ DbUri u=parse_context(context);
  string op=lower_word(query);
  string sql=(op=="select" || op=="with" || op=="show") ? with_order(query,orderby) : query;
  (void)sql;

  if (u.engine=="mysql")
  {
#if EWB_DB_WITH_MYSQL
    MYSQL *db=mysql_open(u);
    if (mysql_query(db,sql.c_str()))
    { string e=mysql_error_string(db);
      mysql_close(db);
      raiseerr(e);
    }

    string r;
    if (op=="select" || op=="with" || op=="show")
      r=mysql_first_record(db);
    else if (op=="insert")
      r=to_string((unsigned long long)mysql_insert_id(db));
    else
      r=to_string((unsigned long long)mysql_affected_rows(db));

    mysql_close(db);
    return r;
#else
    raiseerr("MySQL support not compiled");
#endif
  }

  if (u.engine=="postgres")
  {
#if EWB_DB_WITH_POSTGRES
    PGconn *db=pg_open(u);
    PGresult *res=PQexec(db,sql.c_str());
    ExecStatusType st=PQresultStatus(res);

    string r;
    if (st==PGRES_TUPLES_OK)
      r=pg_first_record(res);
    else if (st==PGRES_COMMAND_OK)
    { char *n=PQcmdTuples(res);
      r=(n && n[0]) ? n : "0";
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
