#include "../db_uri.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

static int check(string text, string engine, string host, string database,
                 unsigned int port)
{
  DbUri uri;
  string error;
  if (!parse_db_uri(text,&uri,&error))
  { cerr << text << ": " << error << "\n";
    return 1;
  }
  if (uri.engine!=engine || uri.host!=host || uri.database!=database || uri.port!=port)
  { cerr << text << ": got " << uri.engine << " " << uri.host << " "
         << uri.database << " " << uri.port << "\n";
    return 1;
  }
  return 0;
}

int main()
{
  int failed=0;
  failed+=check("mysql://ewb","mysql","","ewb",3306);
  failed+=check("mysql://archive:3307","mysql","","archive",3307);
  failed+=check("mysql://192.168.2.51/ewb","mysql","192.168.2.51","ewb",3306);
  failed+=check("mysql://db.internal:3308/family","mysql","db.internal","family",3308);
  failed+=check("postgresql://pg.internal/family","postgres","pg.internal","family",5432);
  DbUri first;
  DbUri second;
  string error;
  parse_db_uri("mysql://db-one/ewb",&first,&error);
  parse_db_uri("mysql://db-two/archive",&second,&error);
  setenv("EWB_MYSQL_HOST","environment-host",1);
  if (db_uri_host(first,"EWB_MYSQL_HOST","EWB_DB_HOST","localhost")!="db-one") failed++;
  if (db_uri_host(second,"EWB_MYSQL_HOST","EWB_DB_HOST","localhost")!="db-two") failed++;
  return failed ? 1 : 0;
}
