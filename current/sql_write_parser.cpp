#include "sql_write_parser.h"

#include <cctype>

using namespace std;

static string trim(string value)
{ size_t first=0;
  while (first<value.size() && isspace((unsigned char)value[first])) first++;
  size_t last=value.size();
  while (last>first && isspace((unsigned char)value[last-1])) last--;
  return value.substr(first,last-first);
}

static string lowercase(string value)
{ for (char &c: value) c=(char)tolower((unsigned char)c);
  return value;
}

static string first_word(string sql)
{ sql=trim(sql);
  size_t end=0;
  while (end<sql.size() && !isspace((unsigned char)sql[end])) end++;
  return lowercase(sql.substr(0,end));
}

static size_t keyword(const string &sql, string word, size_t start=0)
{ string lower=lowercase(sql);
  word=lowercase(word);
  bool quoted=false;
  int parentheses=0;

  for (size_t i=start; i+word.size()<=sql.size(); i++)
  { char c=sql[i];
    if (quoted)
    { if (c=='\'' && i+1<sql.size() && sql[i+1]=='\'') { i++; continue; }
      if (c=='\'') quoted=false;
      continue;
    }
    if (c=='\'') { quoted=true; continue; }
    if (c=='(') { parentheses++; continue; }
    if (c==')') { if (parentheses>0) parentheses--; continue; }
    if (parentheses || lower.compare(i,word.size(),word)!=0) continue;

    bool left=i==0 || !isalnum((unsigned char)lower[i-1]);
    bool right=i+word.size()==lower.size() ||
               !isalnum((unsigned char)lower[i+word.size()]);
    if (left && right) return i;
  }
  return string::npos;
}

static size_t closing_parenthesis(const string &sql, size_t open)
{ bool quoted=false;
  int depth=0;
  for (size_t i=open; i<sql.size(); i++)
  { char c=sql[i];
    if (quoted)
    { if (c=='\'' && i+1<sql.size() && sql[i+1]=='\'') { i++; continue; }
      if (c=='\'') quoted=false;
      continue;
    }
    if (c=='\'') quoted=true;
    else if (c=='(') depth++;
    else if (c==')' && --depth==0) return i;
  }
  return string::npos;
}

static vector<string> split_list(const string &list)
{ vector<string> parts;
  bool quoted=false;
  int parentheses=0;
  size_t begin=0;

  for (size_t i=0; i<list.size(); i++)
  { char c=list[i];
    if (quoted)
    { if (c=='\'' && i+1<list.size() && list[i+1]=='\'') { i++; continue; }
      if (c=='\'') quoted=false;
      continue;
    }
    if (c=='\'') quoted=true;
    else if (c=='(') parentheses++;
    else if (c==')' && parentheses>0) parentheses--;
    else if (c==',' && parentheses==0)
    { parts.push_back(trim(list.substr(begin,i-begin)));
      begin=i+1;
    }
  }
  parts.push_back(trim(list.substr(begin)));
  return parts;
}

static string field_name(string field)
{ field=trim(field);
  size_t dot=field.rfind('.');
  if (dot!=string::npos) field=trim(field.substr(dot+1));
  if (field.size()>=2 &&
      ((field.front()=='`' && field.back()=='`') ||
       (field.front()=='"' && field.back()=='"')))
    field=field.substr(1,field.size()-2);
  return field;
}

static bool string_literal(string expression, string &value)
{ expression=trim(expression);
  if (expression.size()<2 || expression.front()!='\'' || expression.back()!='\'')
    return false;

  value.clear();
  for (size_t i=1; i+1<expression.size(); i++)
  { char c=expression[i];
    if (c=='\'' && i+1<expression.size()-1 && expression[i+1]=='\'')
    { value+='\'';
      i++;
    }
    else value+=c;
  }
  return true;
}

static vector<SqlWriteValue> insert_values(const string &sql)
{ vector<SqlWriteValue> result;
  size_t fields_open=sql.find('(');
  if (fields_open==string::npos) return result;
  size_t fields_close=closing_parenthesis(sql,fields_open);
  if (fields_close==string::npos) return result;
  size_t values_word=keyword(sql,"values",fields_close+1);
  if (values_word==string::npos) return result;
  size_t values_open=sql.find('(',values_word+6);
  if (values_open==string::npos) return result;
  size_t values_close=closing_parenthesis(sql,values_open);
  if (values_close==string::npos) return result;

  vector<string> fields=split_list(sql.substr(fields_open+1,fields_close-fields_open-1));
  vector<string> values=split_list(sql.substr(values_open+1,values_close-values_open-1));
  if (fields.size()!=values.size()) return result;

  for (size_t i=0; i<fields.size(); i++)
  { string value;
    if (string_literal(values[i],value))
      result.push_back({field_name(fields[i]),value});
  }
  return result;
}

static vector<SqlWriteValue> update_values(const string &sql)
{ vector<SqlWriteValue> result;
  size_t set_word=keyword(sql,"set");
  if (set_word==string::npos) return result;
  size_t end=keyword(sql,"where",set_word+3);
  if (end==string::npos) end=sql.size();

  for (string assignment: split_list(sql.substr(set_word+3,end-set_word-3)))
  { size_t equal=assignment.find('=');
    if (equal==string::npos) continue;
    string value;
    if (string_literal(assignment.substr(equal+1),value))
      result.push_back({field_name(assignment.substr(0,equal)),value});
  }
  return result;
}

vector<SqlWriteValue> sql_write_values(const string &sql)
{ string operation=first_word(sql);
  if (operation=="insert") return insert_values(sql);
  if (operation=="update") return update_values(sql);
  return vector<SqlWriteValue>();
}

size_t utf8_characters(const string &value)
{ size_t count=0;
  for (unsigned char c: value)
    if ((c & 0xc0)!=0x80) count++;
  return count;
}
