#include "db_uri.h"

#include <cstdlib>

using namespace std;

static bool parse_port(string text, unsigned int *port)
{
  if (text=="") return false;
  char *end=NULL;
  unsigned long value=strtoul(text.c_str(),&end,10);
  if (!end || *end || !value || value>65535) return false;
  *port=(unsigned int)value;
  return true;
}

bool parse_db_uri(string context, DbUri *uri, string *error)
{
  DbUri u;
  u.engine="mysql";
  u.host="";
  u.database="ewb";
  u.port=3306;

  size_t scheme=context.find("://");
  if (scheme==string::npos)
  { if (error) *error="DB context URI";
    return false;
  }

  u.engine=context.substr(0,scheme);
  string rest=context.substr(scheme+3);
  if (u.engine=="mysql") u.port=3306;
  else if (u.engine=="postgres" || u.engine=="postgresql")
  { u.engine="postgres";
    u.port=5432;
  }
  else
  { if (error) *error="DB engine";
    return false;
  }

  size_t slash=rest.find('/');
  if (slash==string::npos)
  {
    // Forma storica EWB: engine://database[:port].
    if (rest!="")
    { size_t colon=rest.rfind(':');
      if (colon==string::npos) u.database=rest;
      else
      { u.database=rest.substr(0,colon);
        if (!parse_port(rest.substr(colon+1),&u.port))
        { if (error) *error="DB context URI";
          return false;
        }
      }
    }
  }
  else
  {
    // Forma distribuita: engine://host[:port]/database.
    string authority=rest.substr(0,slash);
    u.database=rest.substr(slash+1);
    if (authority=="")
    { if (error) *error="DB context URI";
      return false;
    }
    size_t colon=authority.rfind(':');
    if (colon==string::npos) u.host=authority;
    else
    { u.host=authority.substr(0,colon);
      if (u.host=="" || !parse_port(authority.substr(colon+1),&u.port))
      { if (error) *error="DB context URI";
        return false;
      }
    }
  }

  if (u.database=="") u.database="ewb";
  *uri=u;
  return true;
}

string db_uri_host(const DbUri &uri, const char *engine_env,
                   const char *shared_env, const char *fallback)
{
  if (uri.host!="") return uri.host;
  const char *value=getenv(engine_env);
  if (value && value[0]) return value;
  value=getenv(shared_env);
  if (value && value[0]) return value;
  return fallback;
}
