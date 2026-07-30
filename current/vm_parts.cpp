#include "vm_parts.h"

#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <limits>
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

static int ewbDigit(char c)
{ if (c>='0' && c<='9') return c-'0';
  if (c>='a' && c<='f') return c-'a'+10;
  if (c>='A' && c<='F') return c-'A'+10;
  return -1;
}

static bool ewbParseNumber(const string &text,double *result)
{ if (text=="") return false;
  size_t pos=0;
  int sign=1;
  if (text[pos]=='+' || text[pos]=='-')
  { if (text[pos]=='-') sign=-1;
    pos++;
  }
  if (pos>=text.size()) return false;

  int base=10;
  if (pos+2<=text.size() && text[pos]=='0')
  { if (text[pos+1]=='b') { base=2; pos+=2; }
    else if (text[pos+1]=='o') { base=8; pos+=2; }
    else if (text[pos+1]=='x') { base=16; pos+=2; }
  }
  if (pos>=text.size()) return false;

  long double value=0;
  bool digit=false;
  while (pos<text.size() && text[pos]!='.')
  { int d=ewbDigit(text[pos]);
    if (d<0 || d>=base) return false;
    value=value*base+d;
    digit=true;
    pos++;
  }
  if (!digit) return false;

  if (pos<text.size() && text[pos]=='.')
  { pos++;
    long double place=1.0L/base;
    while (pos<text.size())
    { int d=ewbDigit(text[pos]);
      if (d<0 || d>=base) return false;
      value+=d*place;
      place/=base;
      pos++;
    }
  }
  if (pos!=text.size()) return false;
  value*=sign;
  if (!isfinite(value)) return false;
  if (value>numeric_limits<double>::max() ||
      value<-numeric_limits<double>::max()) return false;
  *result=(double)value;
  return isfinite(*result);
}

int ewbIntValue(string v)
{ double number=0;
  if (!ewbParseNumber(v,&number)) { raiseerr("Number"); return 0; }
  number=trunc(number);
  if (number>numeric_limits<int>::max() ||
      number<numeric_limits<int>::min())
  { raiseerr("Integer range");
    return 0;
  }
  return (int)number;
}

double ewbValue(string v)
{ double result=0;
  if (!ewbParseNumber(v,&result)) { raiseerr("Number"); return 0; }
  return result;
}

bool ewbTrue(string v)
{ if (v=="") return false;
  double number=0;
  if (ewbParseNumber(v,&number)) return number!=0;
  return true;
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
{ return ewbBool(ewbTrue(x) || ewbTrue(y));
}

string ewbAnd(string x, string y)
{ return ewbBool(ewbTrue(x) && ewbTrue(y));
}

string ewbNot(string x)
{ return ewbBool(!ewbTrue(x));
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

struct EwbArrayEntry
{ string key;
  string value;
};

static vector<EwbArrayEntry> ewbArrayDecode(string value)
{ vector<EwbArrayEntry> entries;
  if (value=="") return entries;
  if ((unsigned char)value[0]!=0x1e) raiseerr("Array expected");
  if (value.size()==1) return entries;
  if (value[1]==' ' || value.back()==' ') raiseerr("Array format");

  size_t begin=1;
  while (begin<value.size())
  { size_t end=value.find(' ',begin);
    if (end==string::npos) end=value.size();
    if (end==begin) raiseerr("Array format");
    string token=value.substr(begin,end-begin);
    size_t colon=token.find(':');
    if (colon==string::npos || token.find(':',colon+1)!=string::npos)
      raiseerr("Array format");

    EwbArrayEntry entry;
    entry.key=hexDecode(token.substr(0,colon));
    entry.value=hexDecode(token.substr(colon+1));
    for (size_t i=0; i<entries.size();)
    { if (entries[i].key==entry.key) entries.erase(entries.begin()+i);
      else i++;
    }
    entries.push_back(entry);
    if (end==value.size()) break;
    begin=end+1;
  }
  return entries;
}

static string ewbArrayEncode(const vector<EwbArrayEntry> &entries)
{ string result;
  result+=(char)0x1e;
  for (size_t i=0; i<entries.size(); i++)
  { if (i) result+=" ";
    result+=hexEncode(entries[i].key);
    result+=":";
    result+=hexEncode(entries[i].value);
  }
  return result;
}

static bool ewbNumericKey(string key,int *number)
{ if (key=="0")
  { *number=0;
    return true;
  }
  if (key=="" || key[0]=='0') return false;
  long long value=0;
  for (size_t i=0; i<key.size(); i++)
  { if (key[i]<'0' || key[i]>'9') return false;
    value=value*10+key[i]-'0';
    if (value>numeric_limits<int>::max()) return false;
  }
  *number=(int)value;
  return true;
}

bool ewbIsArray(string value)
{ return value!="" && (unsigned char)value[0]==0x1e;
}

int ewbNumKey(string value)
{ return (int)ewbArrayDecode(value).size();
}

int ewbNumEl(string value)
{ vector<EwbArrayEntry> entries=ewbArrayDecode(value);
  int count=0;
  for (size_t i=0; i<entries.size(); i++)
  { int number;
    if (ewbNumericKey(entries[i].key,&number)) count++;
  }
  return count;
}

string ewbArrayPush(string *value, string element)
{ vector<EwbArrayEntry> entries=ewbArrayDecode(*value);
  unsigned long long next=0;
  for (size_t i=0; i<entries.size(); i++)
  { int number;
    if (!ewbNumericKey(entries[i].key,&number)) continue;
    unsigned long long key=(unsigned long long)number;
    if (key>=next) next=key+1;
  }
  vector<string> path(1,to_string(next));
  *value=ewbSetPath(*value,element,path);
  return element;
}

string ewbSplit(string separator, string value)
{ string result="";
  if (ewbIsArray(value))
  { vector<EwbArrayEntry> entries=ewbArrayDecode(value);
    for (size_t i=0; i<entries.size(); i++)
    { string item=entries[i].value;
      int number;
      if (!ewbNumericKey(entries[i].key,&number))
      { string pair="";
        vector<string> first(1,"0");
        vector<string> second(1,"1");
        pair=ewbSetPath(pair,entries[i].key,first);
        pair=ewbSetPath(pair,entries[i].value,second);
        item=pair;
      }
      ewbArrayPush(&result,item);
    }
    return result;
  }

  if (value=="") return string(1,(char)0x1e);
  if (separator=="")
  { for (size_t i=0; i<value.size(); i++)
      ewbArrayPush(&result,value.substr(i,1));
    return result;
  }

  size_t begin=0;
  for (;;)
  { size_t end=value.find(separator,begin);
    if (end==string::npos)
    { ewbArrayPush(&result,value.substr(begin));
      break;
    }
    ewbArrayPush(&result,value.substr(begin,end-begin));
    begin=end+separator.size();
  }
  return result;
}

string ewbJoin(string separator, string value)
{ vector<EwbArrayEntry> entries=ewbArrayDecode(value);
  bool structured=false;
  for (size_t i=0; i<entries.size(); i++)
  { if (ewbIsArray(entries[i].value))
    { vector<EwbArrayEntry> pair=ewbArrayDecode(entries[i].value);
      if (pair.size()==2 && pair[0].key=="0" && pair[1].key=="1")
        structured=true;
    }
  }

  if (structured)
  { string result="";
    for (size_t i=0; i<entries.size(); i++)
    { string item=entries[i].value;
      if (ewbIsArray(item))
      { vector<EwbArrayEntry> pair=ewbArrayDecode(item);
        if (pair.size()==2 && pair[0].key=="0" && pair[1].key=="1")
        { vector<string> path(1,pair[0].value);
          result=ewbSetPath(result,pair[1].value,path);
          continue;
        }
      }
      ewbArrayPush(&result,item);
    }
    return result;
  }

  string result="";
  for (size_t i=0; i<entries.size(); i++)
  { if (i) result+=separator;
    result+=entries[i].value;
  }
  return result;
}

string ewbKeyAt(string value,int position)
{ vector<EwbArrayEntry> entries=ewbArrayDecode(value);
  if (position<0 || position>=(int)entries.size()) return "";
  return entries[position].key;
}

string ewbHasKey(string value,string key)
{ vector<EwbArrayEntry> entries=ewbArrayDecode(value);
  for (size_t i=0; i<entries.size(); i++)
    if (entries[i].key==key) return "1";
  return "0";
}

string ewbGetElement(string value,string key)
{ vector<EwbArrayEntry> entries=ewbArrayDecode(value);
  for (size_t i=0; i<entries.size(); i++)
    if (entries[i].key==key) return entries[i].value;
  return "";
}

static string ewbSetElement(string value,string key,string element)
{ vector<EwbArrayEntry> entries=ewbArrayDecode(value);
  for (size_t i=0; i<entries.size(); i++)
  { if (entries[i].key==key)
    { entries[i].value=element;
      return ewbArrayEncode(entries);
    }
  }
  EwbArrayEntry entry;
  entry.key=key;
  entry.value=element;
  entries.push_back(entry);
  return ewbArrayEncode(entries);
}

string ewbSetPath(string value,string element,vector<string> path)
{ if (path.empty()) return element;
  string child=ewbGetElement(value,path[0]);
  vector<string> remaining(path.begin()+1,path.end());
  child=ewbSetPath(child,element,remaining);
  return ewbSetElement(value,path[0],child);
}

string ewbGetPath(string value,vector<string> path)
{ if (path.empty()) return value;
  string child=ewbGetElement(value,path[0]);
  if (child=="" && ewbHasKey(value,path[0])=="0") return "";
  vector<string> remaining(path.begin()+1,path.end());
  return ewbGetPath(child,remaining);
}

string ewbDeleteKey(string *value,string key)
{ vector<EwbArrayEntry> entries=ewbArrayDecode(*value);
  string removed="";
  for (size_t i=0; i<entries.size(); i++)
  { if (entries[i].key==key)
    { removed=entries[i].value;
      entries.erase(entries.begin()+i);
      *value=ewbArrayEncode(entries);
      return removed;
    }
  }
  return "";
}

string ewbArrayPop(string *value)
{ vector<EwbArrayEntry> entries=ewbArrayDecode(*value);
  int best=-1;
  int best_number=-1;
  for (size_t i=0; i<entries.size(); i++)
  { int number;
    if (ewbNumericKey(entries[i].key,&number) && number>best_number)
    { best=(int)i;
      best_number=number;
    }
  }
  if (best<0) return "";
  string removed=entries[best].value;
  entries.erase(entries.begin()+best);
  *value=ewbArrayEncode(entries);
  return removed;
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


