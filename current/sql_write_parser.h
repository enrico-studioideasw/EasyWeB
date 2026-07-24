#ifndef EWB_SQL_WRITE_PARSER_H
#define EWB_SQL_WRITE_PARSER_H

#include <string>
#include <vector>

struct SqlWriteValue
{ std::string field;
  std::string value;
};

std::vector<SqlWriteValue> sql_write_values(const std::string &sql);
size_t utf8_characters(const std::string &value);

#endif
