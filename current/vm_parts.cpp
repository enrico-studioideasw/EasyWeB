#include "vm_parts.h"

#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstdint>
#include "cron_parser.h"
#include "db_interface.h"

void raiseerr(string e);

string hexEncode(string v)
{ static const char *h = "0123456789ABCDEF";
  string r;
  for (unsigned char c : v)
  { r += h[(c >> 4) & 15];
    r += h[c & 15];
  }
  return r;
}

string hexDecode(string v)
{ auto val = [](char c)->int
  { if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };

  if (v.size() % 2) raiseerr("Hex length");

  string r;
  for (size_t i = 0; i < v.size(); i += 2)
  { int a = val(v[i]);
    int b = val(v[i + 1]);
    if (a < 0 || b < 0) raiseerr("Hex char");
    r += char((a << 4) | b);
  }
  return r;
}

int ewbIntValue(string v)
{ try
  { return stoi(v);
  }
  catch (...)
  { raiseerr("Number");
  }
  return 0;
}

double ewbValue(string v)
{ try
  { return stod(v);
  }
  catch (...)
  { raiseerr("Number");
  }
  return 0;
}

string ewbNumber(double v)
{ ostringstream ss;
  ss << setprecision(15) << v;
  return ss.str();
}

string ewbInt(int v)
{ return to_string(v);
}

string ewbBool(bool v)
{ if (v) return "1";
  return "0";
}

string ewbSum(string x, string y)
{ return ewbNumber(ewbValue(x) + ewbValue(y));
}

string ewbSub(string x, string y)
{ return ewbNumber(ewbValue(x) - ewbValue(y));
}

string ewbNegative(string x)
{ return ewbNumber(-ewbValue(x));
}

string ewbMul(string x, string y)
{ return ewbNumber(ewbValue(x) * ewbValue(y));
}

string ewbDiv(string x, string y)
{ double b = ewbValue(y);
  if (b == 0) raiseerr("Div zero");
  return ewbNumber(ewbValue(x) / b);
}
string ewbMod(string x, string y)
{ int b = ewbIntValue(y);
  if (b == 0) raiseerr("Div zero");
  return ewbInt(ewbIntValue(x) % b);
}

string ewbOr(string x, string y)
{ return ewbBool(ewbIntValue(x) || ewbIntValue(y));
}

string ewbAnd(string x, string y)
{ return ewbBool(ewbIntValue(x) && ewbIntValue(y));
}

string ewbNot(string x)
{ return ewbBool(!ewbIntValue(x));
}

string ewbBitwiseOr(string x, string y)
{ return ewbInt(ewbIntValue(x) | ewbIntValue(y));
}

string ewbBitwiseAnd(string x, string y)
{ return ewbInt(ewbIntValue(x) & ewbIntValue(y));
}

string ewbBitwiseNot(string x)
{ return ewbInt(~ewbIntValue(x));
}

string ewbCompare(string x, string y, string cmp)
{ if (cmp[0]=='s')
  { if (cmp == "sgt")  return ewbBool(x > y);
    if (cmp == "slt")  return ewbBool(x < y);
    if (cmp == "sge")  return ewbBool(x >= y);
    if (cmp == "sle")  return ewbBool(x <= y);
    if (cmp == "seq")  return ewbBool(x == y);
    if (cmp == "sneq") return ewbBool(x != y);
  } else
  { double a = ewbValue(x);
    double b = ewbValue(y);
    if (cmp == "gt")  return ewbBool(a > b);
    if (cmp == "lt")  return ewbBool(a < b);
    if (cmp == "ge")  return ewbBool(a >= b);
    if (cmp == "le")  return ewbBool(a <= b);
    if (cmp == "eq")  return ewbBool(a == b);
    if (cmp == "neq") return ewbBool(a != b);
  };
  raiseerr("Compare");
  return "0";
}

string togliVirgolette(string s)
{ if (s.size() < 2 || s.front() != '"' || s.back() != '"') raiseerr("Quote");

  string r;
  for (size_t i = 1; i < s.size() - 1; i++)
  { if (s[i] == '\\' && i + 1 < s.size() - 1)
    { char n = s[++i];
      if (n == '"') r += '"';
      else if (n == '\\') r += '\\';
      else if (n == 'n') r += '\n';
      else if (n == 'r') r += '\r';
      else if (n == 't') r += '\t';
      else
      { r += '\\';
        r += n;
      }
    }
    else r += s[i];
  }
  return r;
}

// Legge una stringa binaria quotata e delega gli escape a togliVirgolette().
string readBinaryString(const unsigned char *program, size_t len, size_t *pos)
{ string r;
  if (*pos>=len || program[*pos]!='"') raiseerr("Bad binary string");
  r += '"';
  (*pos)++;
  while (*pos<len)
  { unsigned char c=program[*pos];
    r += (char)c;
    (*pos)++;
    if (c=='\\')
    { if (*pos>=len) raiseerr("Bad binary escape");
      r += (char)program[*pos];
      (*pos)++;
    } else if (c=='"') return togliVirgolette(r);
  }
  raiseerr("Unclosed binary string");
  return "";
}

// Legge un intero binario: solo cifre ASCII, senza delimitatore.
string readBinaryInt(const unsigned char *program, size_t len, size_t *pos)
{ string r;
  if (*pos>=len || program[*pos]<'0' || program[*pos]>'9') raiseerr("Bad binary int");
  while (*pos<len && program[*pos]>='0' && program[*pos]<='9')
  { r += (char)program[*pos];
    (*pos)++;
  }
  return r;
}

string mettiVirgolette(string s)
{ string r = "\"";
  for (char c : s)
  { if (c == '"') r += "\\\"";
    else if (c == '\\') r += "\\\\";
    else if (c == '\n') r += "\\n";
    else if (c == '\r') r += "\\r";
    else if (c == '\t') r += "\\t";
    else r += c;
  }
  r += "\"";
  return r;
}

string signature(string s)
{ uint32_t h = 2166136261u;
  for (unsigned char c : s)
  { h ^= c;
    h *= 16777619u;
  }

  stringstream ss;
  ss << uppercase << hex << setw(8) << setfill('0') << h;
  return ss.str();
}


string escapeTag(string s)
{ string r;
  for (size_t i = 0; i < s.size(); )
  { if (i + 10 <= s.size())
    { string t = s.substr(i, 10);
      for (char &c : t) c = tolower((unsigned char)c);

      if (t == "</textarea")
      { r += "&lt;/textarea";
        i += 10;
        continue;
      }
    }
    r += s[i++];
  }
  return r;
}

int validCronString(string s)
{ ewb_cron cron;
  char err[128];
  return cron_parse(s.c_str(), &cron, err, sizeof(err));
}

void create_base_tables(string url, string user, string password)
{ string maxs=to_string(MAXCONTEXTSTRINGSIZE);
  vector<string> fields;

  fields.push_back("task");
  fields.push_back("program_url");
  fields.push_back("stack");
  fields.push_back("parameters");
  fields.push_back("cronstring");
  run_query(url,user,password,"_crontab",fields,
    "create table if not exists _crontab ("
    "task varchar(" + maxs + ") primary key, "
    "program_url varchar(" + maxs + "), "
    "stack text, "
    "parameters varchar(" + maxs + "), "
    "cronstring varchar(" + maxs + ")"
    ")", "");

  fields.clear();
  fields.push_back("name");
  fields.push_back("status");
  fields.push_back("starttime");
  run_query(url,user,password,"_tasks",fields,
    "create table if not exists _tasks ("
    "name varchar(" + maxs + ") primary key, "
    "status varchar(" + maxs + "), "
    "starttime bigint"
    ")", "");

  run_query(url,user,password,"_threads",fields,
    "create table if not exists _threads ("
    "name varchar(" + maxs + ") primary key, "
    "status varchar(" + maxs + "), "
    "starttime bigint"
    ")", "");
}

vector<string> split(string s, char sep)
{ vector<string> out;
  string part;
  for (char c : s)
  { if (c == sep)
    { out.push_back(part);
      part = "";
    }
    else part += c;
  }
  out.push_back(part);
  return out;
}

string join(vector<string> v, string sep)
{ string r;
  for (size_t i = 0; i < v.size(); i++)
  { if (i) r += sep;
    r += v[i];
  }
  return r;
}

// ------------------------------------------------------------
// Primitive SQL EWB/MariaDB.
// All'esterno escono solo tipodb e tipocursore.
// Dentro questo modulo resta il dettaglio MYSQL/MYSQL_RES.
// ------------------------------------------------------------

#if __has_include(<mysql/mysql.h>)
#include <mysql/mysql.h>
#define EWB_HAS_MYSQL 1
#else
#define EWB_HAS_MYSQL 0
typedef void MYSQL;
typedef void MYSQL_RES;
typedef void* MYSQL_ROW;
#endif

struct ewb_db
{ MYSQL *conn;
};

struct ewb_cursor
{ ewb_db *db;
  MYSQL_RES *res;
};

typedef ewb_db* tipodb;
typedef ewb_cursor* tipocursore;

#if EWB_HAS_MYSQL
static string sql_err(MYSQL *db)
{
  if (!db) return "SQL";
  const char *e = mysql_error(db);
  if (!e || !e[0]) return "SQL";
  return string("SQL ") + e;
}
#endif


// ---- SQL URI parser ----
// Accepted:
//   mysql://                  -> mysql, localhost, 3306
//   mysql://server            -> mysql, server,    3306
//   mysql://server:4005       -> mysql, server,    4005
static void parseSqlUri(string uri,
                        string &engine,
                        string &host,
                        unsigned int &port)
{
  engine = "mysql";
  host   = "localhost";
  port   = 3306;

  size_t p = uri.find("://");
  if (p == string::npos) raiseerr("SQL URI");

  engine = uri.substr(0, p);
  string addr = uri.substr(p + 3);

  if (engine == "") raiseerr("SQL URI");

  if (engine == "mysql") port = 3306;
  else raiseerr("SQL engine");

  if (addr == "") return;

  size_t c = addr.rfind(':');
  if (c == string::npos)
  { host = addr;
    if (host == "") host = "localhost";
    return;
  }

  host = addr.substr(0, c);
  if (host == "") host = "localhost";

  string ps = addr.substr(c + 1);
  if (ps == "") raiseerr("SQL URI");

  port = (unsigned int)atoi(ps.c_str());
  if (!port) raiseerr("SQL URI");
}

tipodb sql_connect(string uri, string user, string pass, string db)
{ string engine, host;
  unsigned int port;

  parseSqlUri(uri, engine, host, port);

  if (engine != "mysql") raiseerr("SQL engine");

#if EWB_HAS_MYSQL
  MYSQL *sql_db = mysql_init(nullptr);
  if (!sql_db) raiseerr("SQL");

  if (!mysql_real_connect(sql_db,
                          host.c_str(),
                          user.c_str(),
                          pass.c_str(),
                          db.c_str(),
                          port,
                          nullptr,
                          0))
  { string e = mysql_error(sql_db);
    mysql_close(sql_db);
    raiseerr("SQL " + e);
  }

  tipodb out = new ewb_db;
  out->conn = sql_db;
  return out;
#else
  (void)user;
  (void)pass;
  (void)db;
  raiseerr("SQL support not compiled");
  return nullptr;
#endif
}

string sql_exec(tipodb db, string q)
{ if (!db || !db->conn) raiseerr("No db");

#if EWB_HAS_MYSQL
  if (mysql_query(db->conn, q.c_str())) raiseerr(sql_err(db->conn));

  return to_string((unsigned long long)mysql_affected_rows(db->conn));
#else
  (void)q;
  raiseerr("SQL support not compiled");
  return "";
#endif
}

tipocursore perform_query(tipodb db, string q)
{ if (!db || !db->conn) raiseerr("No db");

#if EWB_HAS_MYSQL
  if (mysql_query(db->conn, q.c_str())) raiseerr(sql_err(db->conn));

  MYSQL_RES *res = mysql_store_result(db->conn);
  if (!res) raiseerr(sql_err(db->conn));

  tipocursore c = new ewb_cursor;
  c->db = db;
  c->res = res;
  return c;
#else
  (void)q;
  raiseerr("SQL support not compiled");
  return nullptr;
#endif
}

string sql_fetch(tipocursore c)
{ if (!c || !c->res) raiseerr("No cursor");

#if EWB_HAS_MYSQL
  MYSQL_ROW row = mysql_fetch_row(c->res);
  if (!row) return "";

  unsigned int n = mysql_num_fields(c->res);
  unsigned long *len = mysql_fetch_lengths(c->res);

  string r;
  for (unsigned int i = 0; i < n; i++)
  { if (i) r += " ";

    if (row[i]) r += mettiVirgolette(string(row[i], len[i]));
    else        r += "\"\"";
  }

  return r;
#else
  raiseerr("SQL support not compiled");
  return "";
#endif
}

void sql_close(tipocursore c)
{ if (!c) return;

#if EWB_HAS_MYSQL
  if (c->res) mysql_free_result(c->res);
#endif
  delete c;
}

string sqlLastId(tipodb db)
{ if (!db || !db->conn) raiseerr("No db");
#if EWB_HAS_MYSQL
  return to_string((unsigned long long)mysql_insert_id(db->conn));
#else
  raiseerr("SQL support not compiled");
  return "";
#endif
}


