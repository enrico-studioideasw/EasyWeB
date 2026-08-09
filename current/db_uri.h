#ifndef EWB_DB_URI_H
#define EWB_DB_URI_H

#include <string>

struct DbUri
{
  std::string engine;
  std::string host;
  std::string database;
  unsigned int port;
};

bool parse_db_uri(std::string context, DbUri *uri, std::string *error);
std::string db_uri_host(const DbUri &uri, const char *engine_env,
                        const char *shared_env, const char *fallback);

#endif
